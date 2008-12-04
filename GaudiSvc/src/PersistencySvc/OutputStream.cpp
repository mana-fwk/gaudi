// $Id: OutputStream.cpp,v 1.23 2008/01/15 13:46:52 marcocle Exp $
#define GAUDISVC_PERSISTENCYSVC_OUTPUTSTREAM_CPP

// Framework include files
#include "GaudiKernel/Tokenizer.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/IRegistry.h"
#include "GaudiKernel/IAlgManager.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/IConversionSvc.h"
#include "GaudiKernel/IDataManagerSvc.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/IPersistencySvc.h"
#include "GaudiKernel/IOpaqueAddress.h"

#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/strcasecmp.h"
#include "GaudiKernel/DataObject.h"
#include "GaudiKernel/DataStoreItem.h"
#include "OutputStream.h"
#include "OutputStreamAgent.h"

// Define the algorithm factory for the standard output data writer
DECLARE_ALGORITHM_FACTORY(OutputStream)

// Standard Constructor
OutputStream::OutputStream(const std::string& name, ISvcLocator* pSvcLocator)
 : Algorithm(name, pSvcLocator)
{
  m_doPreLoad      = true;
  m_doPreLoadOpt   = false;
  m_verifyItems    = true;
  m_output         = "";
  m_outputName     = "";
  m_outputType     = "UPDATE";
  m_storeName      = "EventDataSvc";
  m_persName       = "EventPersistencySvc";
  m_agent          = new OutputStreamAgent(this);
  m_pDataManager   = 0;
  m_pDataProvider  = 0;
  m_pConversionSvc = 0;
  m_acceptAlgs     = new std::vector<Algorithm*>();
  m_requireAlgs    = new std::vector<Algorithm*>();
  m_vetoAlgs       = new std::vector<Algorithm*>();
  declareProperty("ItemList",         m_itemNames);
  declareProperty("OptItemList",      m_optItemNames);
  declareProperty("Preload",          m_doPreLoad);
  declareProperty("PreloadOptItems",  m_doPreLoadOpt);
  declareProperty("Output",           m_output);
  declareProperty("OutputFile",       m_outputName);
  declareProperty("EvtDataSvc",       m_storeName);
  declareProperty("EvtConversionSvc", m_persName);
  declareProperty("AcceptAlgs",       m_acceptNames);
  declareProperty("RequireAlgs",      m_requireNames);
  declareProperty("VetoAlgs",         m_vetoNames);
  declareProperty("VerifyItems",      m_verifyItems);

  // Associate action handlers with the AcceptAlgs, RequireAlgs & VetoAlgs properties
  m_acceptNames.declareUpdateHandler ( &OutputStream::acceptAlgsHandler , this );
  m_requireNames.declareUpdateHandler( &OutputStream::requireAlgsHandler, this );
  m_vetoNames.declareUpdateHandler   ( &OutputStream::vetoAlgsHandler   , this );
}

// Standard Destructor
OutputStream::~OutputStream()   {
  delete m_agent;
  delete m_acceptAlgs;
  delete m_requireAlgs;
  delete m_vetoAlgs;
}

// initialize data writer
StatusCode OutputStream::initialize() {
  StatusCode status = StatusCode::SUCCESS;
  MsgStream log(msgSvc(), name());

  // Reset the number of events written
  m_events = 0;
  // Get access to the DataManagerSvc
  status = serviceLocator()->service(m_storeName, m_pDataManager, true);
  if( !status.isSuccess() )   {
    log << MSG::FATAL << "Unable to locate IDataManagerSvc interface" << endreq;
    return status;
  }
  // Get access to the assigned data service
  status = serviceLocator()->service(m_storeName, m_pDataProvider, true);
  if( !status.isSuccess() )   {
    log << MSG::FATAL << "Unable to locate IDataProviderSvc interface of " << m_storeName << endreq;
    return status;
  }
  if ( !(m_itemNames.empty() && m_optItemNames.empty()) )  {
    status = connectConversionSvc();
    if( !status.isSuccess() )   {
      log << MSG::FATAL << "Unable to connect to conversion service." << endreq;
      return status;
    }
  }

  // Clear the list with optional items
  clearItems(m_optItemList);
  // Clear the item list
  clearItems(m_itemList);

  ItemNames::iterator i;
  // Take the new item list from the properties.
  for(i = m_itemNames.begin(); i != m_itemNames.end(); i++)   {
    addItem( m_itemList, *i );
  }

  // Take the new item list from the properties.
  for(i = m_optItemNames.begin(); i != m_optItemNames.end(); i++)   {
    addItem( m_optItemList, *i );
  }

  // Take the item list to the data service preload list.
  if ( m_doPreLoad )    {
    for(Items::iterator j = m_itemList.begin(); j != m_itemList.end(); j++)   {
      m_pDataProvider->addPreLoadItem( *(*j) ).ignore();
    }
    // Not working: bad reference counting! pdataSvc->release();
  }

  if ( m_doPreLoadOpt )    {
    for(Items::iterator j=m_optItemList.begin(); j!=m_optItemList.end(); j++) {
      m_pDataProvider->addPreLoadItem( *(*j) );
    }
  }
  log << MSG::INFO << "Data source: " << m_storeName  << " output: " << m_output << endreq;

  // Decode the accept, required and veto Algorithms. The logic is the following:
  //  a. The event is accepted if all lists are empty.
  //  b. The event is provisionally accepted if any Algorithm in the accept list
  //     has been executed and has indicated that its filter is passed. This
  //     provisional acceptance can be overridden by the other lists.
  //  c. The event is rejected unless all Algorithms in the required list have
  //     been executed and have indicated that their filter passed.
  //  d. The event is rejected if any Algorithm in the veto list has been
  //     executed and has indicated that its filter has passed.
  decodeAcceptAlgs ().ignore();
  decodeRequireAlgs().ignore();
  decodeVetoAlgs   ().ignore();
  return status;
}

// terminate data writer
StatusCode OutputStream::finalize() {
  MsgStream log(msgSvc(), name());
  log << MSG::INFO << "Events output: " << m_events << endreq;
  if ( m_pDataProvider ) m_pDataProvider->release();
  m_pDataProvider = 0;
  if ( m_pDataManager ) m_pDataManager->release();
  m_pDataManager = 0;
  if ( m_pConversionSvc ) m_pConversionSvc->release();
  m_pConversionSvc = 0;
  clearItems(m_optItemList);
  clearItems(m_itemList);
	return StatusCode::SUCCESS;
}

// Work entry point
StatusCode OutputStream::execute() {
  // Clear any previously existing item list
  clearSelection();
  // Test whether this event should be output
  if ( isEventAccepted() )  {
    StatusCode sc = writeObjects();
    clearSelection();
    m_events++;
    return sc;
  }
  return StatusCode::SUCCESS;
}

// Select the different objects and write them to file 
StatusCode OutputStream::writeObjects()  {
  // Connect the output file to the service
  StatusCode status = collectObjects();
  if ( status.isSuccess() )   {
    IDataSelector*  sel = selectedObjects();
    if ( sel->begin() != sel->end() )  {
      status = m_pConversionSvc->connectOutput(m_outputName, m_outputType);
      if ( status.isSuccess() )   {
        // Now pass the collection to the persistency service
        IDataSelector::iterator j;
        IOpaqueAddress* pAddress = 0;
        for ( j = sel->begin(); j != sel->end(); j++ )    {
          StatusCode iret = m_pConversionSvc->createRep( *j, pAddress );
          if ( !iret.isSuccess() )      {
            status = iret;
            continue;
          }
          IRegistry* pReg = (*j)->registry();
          pReg->setAddress(pAddress);
        }
        for ( j = sel->begin(); j != sel->end(); j++ )    {
          IRegistry* pReg = (*j)->registry();
          StatusCode iret = m_pConversionSvc->fillRepRefs( pReg->address(), *j );
          if ( !iret.isSuccess() )      {
            status = iret;
          }
        }
	      // Commit the data if there was no error; otherwise possibly discard
        if ( status.isSuccess() )  {
          status = m_pConversionSvc->commitOutput(m_outputName, true);
        }
        else   {
          m_pConversionSvc->commitOutput(m_outputName, false);
        }
      }
    }
  }
  return status;
}

// Place holder to create configurable data store agent
bool OutputStream::collect(IRegistry* dir, int level)    {
  if ( level < m_currentItem->depth() )   {
    if ( dir->object() != 0 )   {
      /*
      std::cout << "Analysing (" 
                << dir->name() 
                << ") Object:" 
                << ((dir->object()==0) ? "UNLOADED" : "LOADED") 
                << std::endl;
      */
      m_objects.push_back(dir->object());
      return true;
    }
  }
  return false;
}

/// Collect all objects to be written tio the output stream
StatusCode OutputStream::collectObjects()   {
  MsgStream log(msgSvc(), name());
  StatusCode status = StatusCode::SUCCESS;
  Items::iterator i;
  // Traverse the tree and collect the requested objects
  for ( i = m_itemList.begin(); i != m_itemList.end(); i++ )    {
    DataObject* obj = 0;
    m_currentItem = (*i);
    StatusCode iret = m_pDataProvider->retrieveObject(m_currentItem->path(), obj);
    if ( iret.isSuccess() )  {
      iret = m_pDataManager->traverseSubTree(obj, m_agent);
      if ( !iret.isSuccess() )  {
        status = iret;
      }
    }
    else  {
      log << MSG::ERROR << "Cannot write mandatory object(s) (Not found) " 
          << m_currentItem->path() << endreq;
      status = iret;
    }
  }
  // Traverse the tree and collect the requested objects (tolerate missing itmes here)
  for ( i = m_optItemList.begin(); i != m_optItemList.end(); i++ )    {
    DataObject* obj = 0;
    m_currentItem = (*i);
    StatusCode iret = m_pDataProvider->retrieveObject(m_currentItem->path(), obj);
    if ( iret.isSuccess() )  {
      iret = m_pDataManager->traverseSubTree(obj, m_agent);
    }
    if ( !iret.isSuccess() )    {
      log << MSG::DEBUG << "Ignore request to write non-mandatory object(s) " 
          << m_currentItem->path() << endreq;
    }
  }
  return status;
}

// Clear collected object list
void OutputStream::clearSelection()     {
  m_objects.erase(m_objects.begin(), m_objects.end());
}

// Remove all items from the output streamer list;
void OutputStream::clearItems(Items& itms)     {
  for ( Items::iterator i = itms.begin(); i != itms.end(); i++ )    {  
    delete (*i);
  }
  itms.erase(itms.begin(), itms.end());
}

// Find single item identified by its path (exact match)
DataStoreItem* 
OutputStream::findItem(const std::string& path)  {
  for(Items::const_iterator i=m_itemList.begin(); i != m_itemList.end(); ++i)  {
    if ( (*i)->path() == path )  return (*i);
  }
  for(Items::const_iterator j=m_optItemList.begin(); j != m_optItemList.end(); ++j)  {
    if ( (*j)->path() == path )  return (*j);
  }
  return 0;
}

// Add item to output streamer list
void OutputStream::addItem(Items& itms, const std::string& descriptor)   {
	MsgStream log(msgSvc(), name());
  int level = 0;
  size_t sep = descriptor.rfind("#");
  std::string obj_path (descriptor,0,sep);
  std::string slevel   (descriptor,sep+1,descriptor.length());
  if ( slevel == "*" )  {
    level = 9999999;
  }
  else   {
    level = atoi(slevel.c_str());
  }
  if ( m_verifyItems )  {
    size_t idx = obj_path.find("/",1);
    while(idx != std::string::npos)  {
      std::string sub_item = obj_path.substr(0,idx);
      if ( 0 == findItem(sub_item) )   {
        addItem(itms, sub_item+"#1");
      }
      idx = obj_path.find("/",idx+1);
    }
  }
  DataStoreItem* item = new DataStoreItem(obj_path, level);
  log << MSG::DEBUG << "Adding OutputStream item " << item->path()
      << " with " << item->depth() 
      << " level(s)." << endreq;
  itms.push_back( item );
}

// Connect to proper conversion service
StatusCode OutputStream::connectConversionSvc()   {
  StatusCode status = StatusCode::FAILURE;
	MsgStream log(msgSvc(), name());
  // Get output file from input
  std::string dbType, svc, shr;
  Tokenizer tok(true);
  tok.analyse(m_output, " ", "", "", "=", "'", "'");
  for(Tokenizer::Items::iterator i = tok.items().begin(); i != tok.items().end(); ++i)   {
    const std::string& tag = (*i).tag();
    const std::string& val = (*i).value();
    switch( ::toupper(tag[0]) )    {
    case 'D':
      m_outputName = val;
      break;
    case 'T':
      dbType = val;
      break;
    case 'S':
      switch( ::toupper(tag[1]) )   {
      case 'V':    svc = val;      break;
      case 'H':    shr = "YES";    break;
      }
      break;
    case 'O':   // OPT='<NEW<CREATE,WRITE,RECREATE>, UPDATE>'
      switch( ::toupper(val[0]) )   {
      case 'R':
        if ( ::strncasecmp(val.c_str(),"RECREATE",3)==0 )
          m_outputType = "RECREATE";
        else if ( ::strncasecmp(val.c_str(),"READ",3)==0 )
          m_outputType = "READ";
        break;
      case 'C':
      case 'N':
      case 'W':
        m_outputType = "NEW";
        break;
      case 'U':
        m_outputType = "UPDATE";
        break;
      default:
        m_outputType = "???";
        break;
      }
      break;
    default:
      break;
    }
  }
  if ( !shr.empty() ) m_outputType += "|SHARED";
  // Get access to the default Persistency service
  // The default service is the same for input as for output.
  // If this is not desired, then a specialized OutputStream must overwrite 
  // this value.
  if ( dbType.length() > 0 && svc.length()==0 )   {
    IPersistencySvc* ipers = 0;
    status = serviceLocator()->service(m_persName, ipers );
    if( !status.isSuccess() )   {
      log << MSG::FATAL << "Unable to locate IPersistencySvc interface of " << m_persName << endreq;
      return status;
    }
    status = ipers->getService(dbType, m_pConversionSvc);
    if( !status.isSuccess() )   {
      log << MSG::FATAL << "Unable to locate IConversionSvc interface of database type " << dbType << endreq;
      return status;
    }
    // Increase reference count and keep service.
    m_pConversionSvc->addRef();
  }
  else if ( svc.length() > 0 )    {
    // On success reference count is automatically increased.
    status = serviceLocator()->service(svc, m_pConversionSvc, true );
    if( !status.isSuccess() )   {
      log << MSG::FATAL << "Unable to locate IConversionSvc interface of " << svc << endreq;
      return status;
    }
  }
  else    {
    log << MSG::FATAL
        << "Unable to locate IConversionSvc interface (Unknown technology) " << endreq
        << "You either have to specify a technology name or a service name!" << endreq
        << "Please correct the job option \"" << name() << ".Output\" !"     << endreq;
    status = StatusCode::FAILURE;
  }
  return status;
}

StatusCode OutputStream::decodeAcceptAlgs( ) {
  return decodeAlgorithms( m_acceptNames, m_acceptAlgs );
}

void OutputStream::acceptAlgsHandler( Property& /* theProp */ )  {
  StatusCode sc = decodeAlgorithms( m_acceptNames, m_acceptAlgs );
  if (sc.isFailure()) {
    throw GaudiException("Failure in OutputStream::decodeAlgorithms",
                         "OutputStream::acceptAlgsHandler",sc);
  }
}

StatusCode OutputStream::decodeRequireAlgs( )  {
  return decodeAlgorithms( m_requireNames, m_requireAlgs );
}

void OutputStream::requireAlgsHandler( Property& /* theProp */ )  {
  StatusCode sc = decodeAlgorithms( m_requireNames, m_requireAlgs );
  if (sc.isFailure()) {
    throw GaudiException("Failure in OutputStream::decodeAlgorithms",
                         "OutputStream::requireAlgsHandler",sc);
  }
}

StatusCode OutputStream::decodeVetoAlgs( )  {
  return decodeAlgorithms( m_vetoNames, m_vetoAlgs );
}

void OutputStream::vetoAlgsHandler( Property& /* theProp */ )  {
  StatusCode sc = decodeAlgorithms( m_vetoNames, m_vetoAlgs );
  if (sc.isFailure()) {
    throw GaudiException("Failure in OutputStream::decodeAlgorithms",
                         "OutputStream::vetoAlgsHandler",sc);
  }
}

StatusCode OutputStream::decodeAlgorithms( StringArrayProperty& theNames,
                                           std::vector<Algorithm*>* theAlgs )
{
  // Reset the list of Algorithms
  theAlgs->clear( );

  MsgStream log( msgSvc( ), name( ) );

  IAlgManager* theAlgMgr;
  StatusCode result = serviceLocator()->getService( "ApplicationMgr",
    IID_IAlgManager,
    *pp_cast<IInterface>(&theAlgMgr) );
  if ( result.isSuccess( ) ) {
    // Build the list of Algorithms from the names list
    const std::vector<std::string> nameList = theNames.value( );
    std::vector<std::string>::const_iterator it;
    std::vector<std::string>::const_iterator itend = nameList.end( );
    for (it = nameList.begin(); it != itend; it++) {
      // Check whether the suppied name corresponds to an existing
      // Algorithm object.
      std::string theName = (*it);
      IAlgorithm* theIAlg;
      Algorithm*  theAlgorithm;
      result = theAlgMgr->getAlgorithm( theName, theIAlg );
      if ( result.isSuccess( ) ) {
        try{
          theAlgorithm = dynamic_cast<Algorithm*>(theIAlg);
        } catch(...){
          result = StatusCode::FAILURE;
        }
      }
      if ( result.isSuccess( ) ) {
        // Check that the specified algorithm doesn't already exist in the list
        std::vector<Algorithm*>::iterator ita;
        std::vector<Algorithm*>::iterator itaend = theAlgs->end( );
        for (ita = theAlgs->begin(); ita != itaend; ita++) {
          Algorithm* existAlgorithm = (*ita);
          if ( theAlgorithm == existAlgorithm ) {
            result = StatusCode::FAILURE;
            break;
          }
        }
        if ( result.isSuccess( ) ) {
          theAlgs->push_back( theAlgorithm );
        }
      }
      else {
        log << MSG::INFO << theName << " doesn't exist - ignored" << endreq;
      }
    }
    result = StatusCode::SUCCESS;
  }
  else {
    log << MSG::FATAL << "Can't locate ApplicationMgr!!!" << endreq;
  }
  return result;
}

bool OutputStream::isEventAccepted( ) const  {
  typedef std::vector<Algorithm*>::iterator AlgIter;
  bool result = true;

  // Loop over all Algorithms in the accept list to see
  // whether any have been executed and have their filter
  // passed flag set. Any match causes the event to be
  // provisionally accepted.
  if ( ! m_acceptAlgs->empty() ) {
    result = false;
    for(AlgIter i=m_acceptAlgs->begin(),end=m_acceptAlgs->end(); i != end; ++i) {
      if ( (*i)->isExecuted() && (*i)->filterPassed() ) {
        result = true;
        break;
      }
    }
  }

  // Loop over all Algorithms in the required list to see
  // whether all have been executed and have their filter
  // passed flag set. Any mismatch causes the event to be
  // rejected.
  if ( result && ! m_requireAlgs->empty() ) {
    for(AlgIter i=m_requireAlgs->begin(),end=m_requireAlgs->end(); i != end; ++i) {
      if ( !(*i)->isExecuted() || !(*i)->filterPassed() ) {
        result = false;
        break;
      }
    }
  }

  // Loop over all Algorithms in the veto list to see
  // whether any have been executed and have their filter
  // passed flag set. Any match causes the event to be
  // rejected.
  if ( result && ! m_vetoAlgs->empty() ) {
    for(AlgIter i=m_vetoAlgs->begin(),end=m_vetoAlgs->end(); i != end; ++i) {
      if ( (*i)->isExecuted() && (*i)->filterPassed() ) {
        result = false;
        break;
      }
    }
  }
  return result;
}