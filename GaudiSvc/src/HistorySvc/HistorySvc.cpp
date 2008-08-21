// $Id: HistorySvc.cpp,v 1.16.2.1 2008/06/19 21:27:23 leggett Exp $

#ifndef GAUDISVC_FASTHISTORYSVC_H
 #include "HistorySvc.h"
#endif

#include "GaudiKernel/HistoryObj.h"

#include "GaudiKernel/JobHistory.h"
#include "GaudiKernel/AlgorithmHistory.h"
#include "GaudiKernel/AlgToolHistory.h"
#include "GaudiKernel/DataHistory.h"
#include "GaudiKernel/ServiceHistory.h"

#ifndef KERNEL_SVCFACTORY_H
 #include "GaudiKernel/SvcFactory.h"
#endif
#ifndef GAUDIKERNEL_ISVCLOCATOR_H
 #include "GaudiKernel/ISvcLocator.h"
#endif

#include "GaudiKernel/System.h"
#include "GaudiKernel/Bootstrap.h"
#include "GaudiKernel/IAlgManager.h"
#include "GaudiKernel/IAlgorithm.h"
#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/IAlgTool.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/IService.h"
#include "GaudiKernel/IJobOptionsSvc.h"
#include "GaudiKernel/IAppMgrUI.h"
#include "GaudiKernel/IIncidentSvc.h"
#include "GaudiKernel/IToolSvc.h"
#include "GaudiKernel/ServiceHandle.h"

#include "GaudiKernel/IAlgContextSvc.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DECLARE_SERVICE_FACTORY(HistorySvc)

using namespace std;

//
///////////////////////////////////////////////////////////////////////////
//

struct DHH {
  CLID id;
  std::string key;
  
  DHH(const CLID& i, const std::string& k):id(i), key(k) {}
  
  bool operator < ( DHH const &rhs ) const {
    if (id != rhs.id) {
      return (id < rhs.id);
    } else {
      return (key < rhs.key);
    }
  } 
};

void fenv_dummy(char**) {}

HistorySvc::HistorySvc( const std::string& name, ISvcLocator* svc )
  : Service( name, svc ), 
    m_isInitialized(false),
    m_dump(false),
    p_algCtxSvc(0),
    m_jobHistory(0),
    m_outputFile(""),
    m_incidentSvc(0),
    m_log(msgSvc(), name )
    
{
    declareProperty("Dump",m_dump);
    declareProperty("Activate", m_activate=true);
    declareProperty("OutputFile",m_outputFile);

    // hack to bring in environ
    vector<string> envtmp = System::getEnv();

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

HistorySvc::~HistorySvc() {
  delete m_jobHistory;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode HistorySvc::reinitialize() {

  clearState();

  m_state = OFFLINE;

  return initialize();

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void HistorySvc::clearState() {
  m_algs.clear();
  std::map<const Algorithm*, AlgorithmHistory*>::iterator algitr;
  for (algitr = m_algmap.begin(); algitr != m_algmap.end(); ++algitr) {
    AlgorithmHistory* h = algitr->second;
    (const_cast<Algorithm*> (algitr->first))->release();
    delete h;
  }
  m_algmap.clear();

  m_ialgtools.clear();
  m_algtools.clear();
  std::map<const AlgTool*, AlgToolHistory*>::iterator atitr;
  for (atitr=m_algtoolmap.begin(); atitr != m_algtoolmap.end(); ++atitr) {
    (const_cast<AlgTool*> (atitr->first))->release();
    delete atitr->second;
  }
  m_algtoolmap.clear();

  m_svcs.clear();
  std::map<const IService*, ServiceHistory*>::iterator svitr;
  for (svitr = m_svcmap.begin(); svitr != m_svcmap.end(); ++svitr) {
    (const_cast<IService*> (svitr->first))->release();
    delete svitr->second;
  }
  m_svcmap.clear();
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode HistorySvc::initialize() {

  StatusCode status = Service::initialize();
  m_log.setLevel( m_outputLevel.value() );

  if (status.isFailure()) {
#ifndef NDEBUG
    m_log << MSG::DEBUG << "Failed to initialize the base class (Service)" 
	  << endreq;
#endif
    return status;
  }

#ifndef NDEBUG
  m_log << MSG::DEBUG << "Initializing HistorySvc" << endreq;
#endif

  if (!m_activate) return StatusCode::SUCCESS;

  static const bool CREATEIF(true);

  if ( service("AlgContextSvc",p_algCtxSvc,CREATEIF).isFailure() ) {
    m_log << MSG::ERROR << "unable to get the AlgContextSvc" << endreq;
    return StatusCode::FAILURE;
  }

  if (service("IncidentSvc", m_incidentSvc, CREATEIF).isFailure()) {
    m_log << MSG::ERROR << "unable to get the IncidentSvc" << endreq;
    return StatusCode::FAILURE;
  }

  // create a weak dependency on the ToolSvc, so that it doesn't get deleted
  // before we're done with it in finalize
  if (ServiceHandle<IToolSvc>("ToolSvc/ToolSvc",name()).retrieve().isFailure()) {
    m_log << MSG::ERROR << "could not retrieve the ToolSvc handle !"
	  << endreq;
    return StatusCode::FAILURE;
  }

  // add listener to be triggered by first BeginEvent with low priority
  // so it gets called first
  const bool rethrow = false;
  const bool oneShot = true; // make the listener called only once 
  m_incidentSvc->addListener(this,IncidentType::BeginEvent,
			     std::numeric_limits<long>::min(),rethrow,oneShot);

  m_isInitialized = true;

  return StatusCode::SUCCESS;

}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode HistorySvc::captureState() {

  if (m_jobHistory == 0) {
    m_jobHistory = new JobHistory;
    IJobOptionsSvc *jo;
    if (service("JobOptionsSvc",jo).isFailure()) {
       m_log << MSG::ERROR 
	     << "Could not get jobOptionsSvc - "
	     << "not adding properties to JobHistory" << endreq;
    } else {

      bool foundAppMgr(false);

      std::vector<std::string> clients = jo->getClients();
      std::vector<std::string>::const_iterator it;
      std::vector<const Property*>::const_iterator itr;
      for (it=clients.begin(); it!=clients.end(); ++it) {
	if (*it == "ApplicationMgr") {
	  foundAppMgr = true;
	}
        const std::vector<const Property*> *props = jo->getProperties(*it);
        for (itr=props->begin(); itr != props->end(); ++itr) {
          m_jobHistory->addProperty( *it, *itr );
        }
      }

      if (!foundAppMgr) {
	IProperty *ap;
	if (service("ApplicationMgr",ap).isFailure()) {
	  m_log << MSG::ERROR << "could not get the ApplicationMgr" << endreq;
	} else {
	  std::vector<Property*>::const_iterator itr2;
	  const std::vector<Property*> props = ap->getProperties();
	  for (itr2=props.begin(); itr2 != props.end(); ++itr2) {
	    m_jobHistory->addProperty( "ApplicationMgr", *itr2 );
	  }
	}
      }

    }
  }

  /// Get all the Algorithms 

  StatusCode sc;
  IAlgManager* algMgr = 0;
  sc = Gaudi::svcLocator()->queryInterface( IID_IAlgManager, 
                                            pp_cast<void>(&algMgr) );
  if (sc.isFailure()) {
    m_log << MSG::ERROR << "Could not get AlgManager" << endreq;
    return StatusCode::FAILURE;
  } else {

    std::list<IAlgorithm*> algs;
    algs = algMgr->getAlgorithms();
    std::list<IAlgorithm*>::const_iterator itr;
    for (itr=algs.begin(); itr!=algs.end(); ++itr) {
      Algorithm* alg = dynamic_cast<Algorithm*> (*itr);
      if (alg == 0) {
	m_log << MSG::WARNING << "Algorithm " << (*itr)->name() 
	      << " does not inherit from Algorithm. Not registering it."
	      << endreq;
      } else {
	registerAlg( *alg ).ignore();
      }
    }
    
    m_log << MSG::INFO;
    m_log << "Registered " << algs.size() << " Algorithms" << endreq;
    
  }

  /// Get all the AlgTools

  m_isInitialized = true;
  std::set<const IAlgTool*>::const_iterator itra;
  for (itra = m_ialgtools.begin(); itra != m_ialgtools.end(); ++itra) {
    (const_cast<IAlgTool*>(*itra))->addRef();
    registerAlgTool(**itra).ignore();
  }

  m_log << MSG::INFO << "Registered " << m_algtools.size() << " AlgTools"
	<< endreq;

  /// Get all the Services

  std::list<IService*> svcs = Gaudi::svcLocator()->getServices();
  
  std::list<IService*>::const_iterator itrs;
  for (itrs=svcs.begin(); itrs!=svcs.end(); ++itrs) {
    (*itrs)->addRef();
    registerSvc(**itrs).ignore();
  }
  
  m_log << MSG::INFO;
  m_log << "Registered " << svcs.size() << " Services" << endreq;

  return StatusCode::SUCCESS;


}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode HistorySvc::finalize() {

  if (!m_activate) return StatusCode::SUCCESS;

  if (m_dump) {
    listProperties().ignore();
  }

  if (m_outputFile != "") {
    std::ofstream ofs;
    ofs.open(m_outputFile.c_str());
    if (!ofs) {
      m_log << MSG::ERROR << "Unable to open output file \"m_outputFile\""
	    << endreq;
    } else {

      //      dumpProperties(ofs);
      dumpState(ofs);

      ofs.close();
    }
  }

  clearState();

  StatusCode status = Service::finalize();

  if ( status.isSuccess() )
    m_log << MSG::INFO << "Service finalised successfully" << endreq;

  return status;

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode HistorySvc::queryInterface(const InterfaceID& riid, 
				      void** ppvInterface) 
{
  StatusCode sc = StatusCode::FAILURE;
  if ( ppvInterface ) {
    *ppvInterface = 0;
    
    if ( IHistorySvc::interfaceID().versionMatch(riid) )    {
      *ppvInterface = static_cast<IHistorySvc*>(this);
      sc = StatusCode::SUCCESS;
      addRef();
    }
    else
      sc = Service::queryInterface( riid, ppvInterface );    
  }
  return sc;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode 
HistorySvc::registerAlg(const Algorithm &alg) {

  JobHistory *job = getJobHistory();
  if (m_algmap.find(&alg) != m_algmap.end()) {
    m_log << MSG::WARNING << "Algorithm " << alg.name()
	  << " already registered with HistorySvc" << endreq;
    return StatusCode::SUCCESS;
  }

  (const_cast<Algorithm*>(&alg))->addRef();

  m_algs.insert(&alg);

  AlgorithmHistory *algHist = new AlgorithmHistory(alg, job);
  m_algmap[&alg] = algHist;

#ifndef NDEBUG
  m_log << MSG::DEBUG << "Registering algorithm: ";
  m_log.setColor(MSG::CYAN);
  m_log << alg.name() << endreq;
  m_log.resetColor();
#endif

  return StatusCode(StatusCode::SUCCESS,true);

}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode 
HistorySvc::listProperties(const Algorithm &alg) const {

  m_log << MSG::INFO << "Dumping properties for " << alg.name() << endl;

  AlgorithmHistory *hist = getAlgHistory( alg );

  if (hist == 0) {
    return StatusCode::FAILURE;
  }

  ostringstream ost;
  ost << *hist;

  std::string str = ost.str();

  m_log << MSG::INFO << alg.name() << " --> " << endl << str << endreq;

  return StatusCode(StatusCode::SUCCESS,true);

}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
HistorySvc::dumpProperties(const Algorithm &alg, std::ofstream& ofs) const {

  AlgorithmHistory *hist = getAlgHistory( alg );

  if (hist == 0) {
    return;
  }

  AlgorithmHistory::PropertyList::const_iterator itr;
  for (itr=hist->properties().begin(); itr!=hist->properties().end(); ++itr) {
    ofs << alg.name() << "  " << dumpProp(*itr) << std::endl;
  }

}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

AlgorithmHistory*
HistorySvc::getAlgHistory(const Algorithm &alg) const {

  const Algorithm *palg = &alg;
  set<const Algorithm*>::const_iterator itr = m_algs.find(palg);
  if ( itr == m_algs.end() ) {
    m_log << MSG::WARNING << "Algorithm " << alg.name() << " not registered"
          << endreq;
    return 0;
  }

  map<const Algorithm*, AlgorithmHistory*>::const_iterator itr2;
  itr2 = m_algmap.find( *itr );

  return ( itr2->second );

}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
HistorySvc::getAlgHistory(std::set<AlgorithmHistory*>& algs) const {

  set<const Algorithm*>::const_iterator itr;
  for (itr=m_algs.begin(); itr!=m_algs.end(); ++itr) {
    AlgorithmHistory *ah = m_algmap.find(*itr)->second;

    algs.insert(ah);
  }

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
StatusCode 
HistorySvc::registerJob() {


  return StatusCode(StatusCode::SUCCESS,true);


}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


StatusCode 
HistorySvc::listProperties() const {

  m_log << MSG::INFO;
  m_log.setColor(MSG::CYAN);
  m_log << "Dumping properties for all Algorithms" << endreq;

  std::map<const Algorithm*, AlgorithmHistory*>::const_iterator itr;
  for (itr=m_algmap.begin(); itr != m_algmap.end(); ++itr) {
    const Algorithm* alg = itr->first;

    listProperties( *alg ).ignore();

  }

  m_log << MSG::INFO;
  m_log.setColor(MSG::CYAN);
  m_log << "Dumping properties for all AlgTools" << endreq;

  std::map<const AlgTool*, AlgToolHistory*>::const_iterator itr_a;
  for (itr_a=m_algtoolmap.begin(); itr_a != m_algtoolmap.end(); ++itr_a) {
    const AlgTool* alg = itr_a->first;

    listProperties( *alg ).ignore();

  }

  m_log << MSG::INFO;
  m_log.setColor(MSG::CYAN);
  m_log << "Dumping properties for all Services" << endreq;

  std::map<const IService*, ServiceHistory*>::const_iterator itr_s;
  for (itr_s=m_svcmap.begin(); itr_s != m_svcmap.end(); ++itr_s) {
    const IService* svc = itr_s->first;

    listProperties( *svc ).ignore();

  }

  m_log << MSG::INFO;
  m_log.setColor(MSG::CYAN);
  m_log << "Dumping properties for Job";
  m_log.resetColor();
  
  ostringstream ost;
  ost << *m_jobHistory;
  std::string str = ost.str();

  m_log << std::endl << str << endreq;
  
  return StatusCode(StatusCode::SUCCESS,true);

}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
HistorySvc::dumpProperties(std::ofstream& ofs) const {


  ofs << "GLOBAL" << std::endl;
  const JobHistory::PropertyList props = m_jobHistory->properties();
  JobHistory::PropertyList::const_iterator itrj;
  for (itrj=props.begin(); itrj != props.end(); ++itrj) {
    std::string client = itrj->first;
    const Property* prp = itrj->second; 
    ofs << client << "  " << dumpProp(prp) << std::endl;
  }
  
  ofs << std::endl << "SERVICES" << std::endl;
  std::map<const IService*, ServiceHistory*>::const_iterator itr_s;
  for (itr_s=m_svcmap.begin(); itr_s != m_svcmap.end(); ++itr_s) {
    const IService* svc = itr_s->first;

    dumpProperties( *svc, ofs );

  }

  ofs << std::endl << "ALGORITHMS" << std::endl;
  std::map<const Algorithm*, AlgorithmHistory*>::const_iterator itr;
  for (itr=m_algmap.begin(); itr != m_algmap.end(); ++itr) {
    const Algorithm* alg = itr->first;

    dumpProperties( *alg, ofs );

  }

  ofs << std::endl << "ALGTOOLS" << std::endl;
  std::map<const AlgTool*, AlgToolHistory*>::const_iterator itr_a;
  for (itr_a=m_algtoolmap.begin(); itr_a != m_algtoolmap.end(); ++itr_a) {
    const AlgTool* alg = itr_a->first;

    dumpProperties( *alg, ofs );

  }


}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

JobHistory*
HistorySvc::getJobHistory() const {

  return m_jobHistory;

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
IAlgorithm*
HistorySvc::getCurrentIAlg() const {
  if (p_algCtxSvc == 0) {
    m_log << MSG::WARNING << "trying to create DataHistoryObj before "
	  << "HistorySvc has been initialized" << endreq;
    return 0;

  } else {
    return p_algCtxSvc->currentAlg();
  }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DataHistory* 
HistorySvc::createDataHistoryObj(const CLID& id, const std::string& key, 
				 const std::string& /* storeName */) {

  if (!m_activate) return 0;
  

  AlgorithmHistory *algHist;

  IAlgorithm* ialg = getCurrentIAlg();
  if (ialg == 0) {
#ifndef NDEBUG
    m_log << MSG::DEBUG
	  << "Could not discover current Algorithm:" << endl
	  << "          object CLID: " << id << "  key: \"" << key 
	  << "\"" << endreq;
#endif
    algHist = 0;
  } else {
    Algorithm* alg = dynamic_cast<Algorithm*>(ialg);
    if (alg != 0) {
      algHist = getAlgHistory( *alg );
    } else {
      m_log << MSG::WARNING
	    << "Could not extract concerete Algorithm:"
	    << endl
	    << "          object CLID: " << id << "  key: \"" << key 
	    << "\"" << endreq;
      algHist = 0;
    }
  }

  DataHistory *hist = new DataHistory(id, key, algHist);

  return hist;

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
StatusCode
HistorySvc::registerDataHistory(const CLID& id, const std::string& key, 
				const std::string& storeName) {

  DHH dhh(id,key);

  pair<DHMitr,DHMitr> mitr = m_datMap.equal_range(dhh);
  
  if (mitr.first == mitr.second) {
    // not in the map
    DataHistory *dh = createDataHistoryObj(id,key,storeName);
    m_datMap.insert(pair<DHH,DataHistory*>(dhh,dh));
  } else {
    // found at least one
    bool match(false);

    std::string algName;
    IAlgorithm *ialg = getCurrentIAlg();
    if (ialg != 0) {
      algName = ialg->name();
    } else {
      algName = "UNKNOWN";
    }

    for (DHMitr itr = mitr.first; itr != mitr.second; ++itr) {
      DataHistory *dh = itr->second;
      if (dh->algorithmHistory()->algorithm_name() == algName) {
	match = true;
	break;
      }
    }

    if (! match) {
      DataHistory *dh = createDataHistoryObj(id,key,storeName);
      m_datMap.insert(pair<DHH,DataHistory*>(dhh,dh));
    }
  }

  return StatusCode::SUCCESS;

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
DataHistory* 
HistorySvc::getDataHistory(const CLID& id, const std::string& key, 
			   const std::string& /*storeName*/) const {

  DHH dhh(id,key);

  pair<DHMCitr,DHMCitr> mitr = m_datMap.equal_range(dhh);

  if(mitr.first == mitr.second) {
    return 0;
  }

  return mitr.first->second;
      
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int
HistorySvc::getDataHistory(const CLID& id, const std::string& key, 
			   const std::string& /*storeName*/,
			   std::list<DataHistory*>& dhlist) const {

  DHH dhh(id,key);

  int n(0);

  pair<DHMCitr,DHMCitr> mitr = m_datMap.equal_range(dhh);

  for (DHMCitr itr=mitr.first; itr != mitr.second; ++itr) {
    dhlist.push_back(itr->second);
    n++;
  }

  return n;

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode 
HistorySvc::registerSvc(const IService &svc) {

  if ( svc.name() == "HistoryStore" ) {
    //    m_log << MSG::WARNING << "not registering store" << endreq;
    return StatusCode(StatusCode::SUCCESS,true);
  }

  JobHistory *job = getJobHistory();
  const IService* psvc = &svc;
  map<const IService*, ServiceHistory*>::const_iterator itr = 
    m_svcmap.find(psvc);
  if (itr == m_svcmap.end()) {

#ifndef NDEBUG
    m_log << MSG::DEBUG << "Registering Service: ";
    m_log.setColor(MSG::CYAN);
    m_log << svc.name() << endreq;
#endif

    m_svcs.insert(psvc);

    ServiceHistory *svcHist = new ServiceHistory(&svc, job);
    m_svcmap[psvc] = svcHist;

    (const_cast<IService*>(psvc))->addRef();

  }

  m_log.resetColor();
  return StatusCode(StatusCode::SUCCESS,true);

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

ServiceHistory*
HistorySvc::getServiceHistory(const IService &svc) const {

  const IService *psvc = &svc;
  map<const IService*, ServiceHistory*>::const_iterator itr = 
    m_svcmap.find(psvc);
  if ( itr == m_svcmap.end() ) {
    m_log << MSG::WARNING << "Service " << svc.name() << " not registered"
          << endreq;
    return 0;
  }

  return ( itr->second );

}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
HistorySvc::getServiceHistory(std::set<ServiceHistory*>& svcs) const {

  set<const IService*>::const_iterator itr;
  for (itr=m_svcs.begin(); itr!=m_svcs.end(); ++itr) {
    ServiceHistory *sh = m_svcmap.find( *itr )->second;

    svcs.insert(sh);
  }

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode 
HistorySvc::listProperties(const IService &svc) const {

  m_log << MSG::INFO << "Dumping properties for " << svc.name() << endl;

  ServiceHistory *hist = getServiceHistory( svc );

  if (hist == 0) {
    return StatusCode::FAILURE;
  }

  ostringstream ost;
  ost << *hist;

  std::string str = ost.str();

  m_log << MSG::INFO << svc.name() << " --> " << endl << str << endreq;
  
  

  return StatusCode(StatusCode::SUCCESS,true);

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
HistorySvc::dumpProperties(const IService &svc, std::ofstream &ofs) const {

  ServiceHistory *hist = getServiceHistory( svc );

  if (hist == 0) {
    return;
  }

  ServiceHistory::PropertyList::const_iterator itr;
  for (itr=hist->properties().begin(); itr != hist->properties().end();++itr) {
    ofs << svc.name() << "  " << dumpProp(*itr) << std::endl;
  }

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
 
StatusCode
HistorySvc::registerAlgTool(const IAlgTool& ialg) {
   
  if (! m_isInitialized) {
    if (p_algCtxSvc == 0) {
      if ( service("AlgContextSvc",p_algCtxSvc,true).isFailure() ) {
	m_log << MSG::ERROR << "unable to get the AlgContextSvc" << endreq;
	return StatusCode::FAILURE;
      }
    }

    m_ialgtools.insert(&ialg);
    return StatusCode::SUCCESS;
  }

  const AlgTool *alg = dynamic_cast<const AlgTool*>( &ialg );
  if ( alg == 0 ) {
    m_log << MSG::ERROR << "Could not dcast IAlgTool \"" << ialg.name()
	  << "\" to an AlgTool" << endreq;
    return StatusCode::FAILURE;
  }

  if (m_algtools.find(alg) != m_algtools.end()) {
    m_log << MSG::WARNING << "AlgTool " << ialg.name() 
	  << " already registered in HistorySvc" << endreq;
    return StatusCode::SUCCESS;
  }

  m_algtools.insert(alg);
  (const_cast<AlgTool*>(alg))->addRef();

  const JobHistory *job = getJobHistory();
  AlgToolHistory *algHist = new AlgToolHistory(*alg, job);
  m_algtoolmap[alg] = algHist;

#ifndef NDEBUG
  m_log << MSG::DEBUG << "Registering algtool: ";
  m_log.setColor(MSG::CYAN);
  m_log << alg->name() << endreq;
  m_log.resetColor();
#endif

  return StatusCode::SUCCESS;

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode 
HistorySvc::listProperties(const IAlgTool& alg) const {

  m_log << MSG::INFO << "Dumping properties for " << alg.name() << endl;

  AlgToolHistory *hist = getAlgToolHistory( alg );

  if (hist == 0) {
    return StatusCode::FAILURE;
  }

  ostringstream ost;
  ost << *hist;

  std::string str = ost.str();

  m_log << MSG::INFO << alg.name() << " --> " << endl << str << endreq;
 
  return StatusCode::SUCCESS;

}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
HistorySvc::dumpProperties(const IAlgTool& alg, std::ofstream &ofs) const {

  AlgToolHistory *hist = getAlgToolHistory( alg );

  if (hist == 0) {
    return;
  }

  AlgToolHistory::PropertyList::const_iterator itr;
  for (itr=hist->properties().begin(); itr!=hist->properties().end(); ++itr) {
    ofs << alg.name() << "  " << dumpProp(*itr) << std::endl;
  }

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

AlgToolHistory* 
HistorySvc::getAlgToolHistory(const IAlgTool& alg) const {

  const AlgTool *palg = dynamic_cast<const AlgTool*>(&alg);
  set<const AlgTool*>::const_iterator itr = m_algtools.find(palg);
  if ( itr == m_algtools.end() ) {
    m_log << MSG::WARNING << "AlgTool " << alg.name() << " not registered"
          << endreq;
    return 0;
  }

  map<const AlgTool*, AlgToolHistory*>::const_iterator itr2;
  itr2 = m_algtoolmap.find( *itr );

  return ( itr2->second );

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void 
HistorySvc::getAlgToolHistory(std::set<AlgToolHistory*>& algs) const {

  set<const AlgTool*>::const_iterator itr;
  for (itr=m_algtools.begin(); itr!=m_algtools.end(); ++itr) {
    AlgToolHistory *ah = m_algtoolmap.find(*itr)->second;

    algs.insert(ah);
  }

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
HistorySvc::handle(const Incident& incident) {

  if (incident.type() == IncidentType::BeginEvent) {
    if (captureState().isFailure()) {
      m_log << MSG::WARNING << "Error capturing state." << endl
            << "Will try again at next BeginEvent incident" << endreq;
    }
  }

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


std::string
HistorySvc::dumpProp(const Property* prop) const {
  std::ostringstream ost;
  prop->fillStream(ost);
  return ost.str();
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
HistorySvc::dumpState(std::ofstream& ofs) const {


  ofs << "GLOBAL" << std::endl;
  const JobHistory::PropertyList props = m_jobHistory->properties();
  JobHistory::PropertyList::const_iterator itrj;
  for (itrj=props.begin(); itrj != props.end(); ++itrj) {
    std::string client = itrj->first;
    const Property* prp = itrj->second; 
    ofs << client << "  " << dumpProp(prp) << std::endl;
  }
  

  ofs << "SERVICES" << std::endl;
  std::map<const IService*, ServiceHistory*>::const_iterator itr_s;
  for (itr_s=m_svcmap.begin(); itr_s != m_svcmap.end(); ++itr_s) {
    const IService* svc = itr_s->first;

    dumpState( *svc, ofs );

  }

  ofs << "ALGORITHMS" << std::endl;
  std::map<const Algorithm*, AlgorithmHistory*>::const_iterator itr;
  for (itr=m_algmap.begin(); itr != m_algmap.end(); ++itr) {
    const Algorithm* alg = itr->first;

    dumpState( *alg, ofs );

  }


  ofs << "ALGTOOLS" << std::endl;
  std::map<const AlgTool*, AlgToolHistory*>::const_iterator itr_a;
  for (itr_a=m_algtoolmap.begin(); itr_a != m_algtoolmap.end(); ++itr_a) {
    const AlgTool* alg = itr_a->first;

    dumpState( *alg, ofs );

  }

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
HistorySvc::dumpState(const Algorithm &alg, std::ofstream& ofs) const {

  AlgorithmHistory *hist = getAlgHistory( alg );

  if (hist == 0) {
    return;
  }

  ofs << ">> " << alg.name() << endl << *hist << endl;

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
HistorySvc::dumpState(const IService &svc, std::ofstream& ofs) const {

  ServiceHistory *hist = getServiceHistory( svc );

  if (hist == 0) {
    return;
  }

  ofs << ">> " << svc.name() << endl << *hist << endl;

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
HistorySvc::dumpState(const IAlgTool &alg, std::ofstream& ofs) const {

  AlgToolHistory *hist = getAlgToolHistory( alg );

  if (hist == 0) {
    return;
  }

  ofs << ">> " << alg.name() << endl << *hist << endl;

}
