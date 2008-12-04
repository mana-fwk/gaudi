// $Header: /tmp/svngaudi/tmp.jEpFh25751/Gaudi/GaudiSvc/src/IncidentSvc/IncidentSvc.cpp,v 1.13 2008/11/10 16:00:23 marcocle Exp $

// Include Files
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/SmartIF.h"
#include "GaudiKernel/IIncidentListener.h"
#include "GaudiKernel/Incident.h"
#include "GaudiKernel/GaudiException.h"
#include "IncidentSvc.h"

// Instantiation of a static factory class used by clients to create
//  instances of this service
DECLARE_SERVICE_FACTORY(IncidentSvc)

namespace {
  // Helper to get the name of the listener
  std::string getListenerName(IIncidentListener* lis)
  {
    SmartIF<INamedInterface> iNamed(lis);
    if (iNamed.isValid()) return iNamed->name();
    else return "<unknown>";
  }
}


//============================================================================================
// Constructors and Desctructors
//============================================================================================
IncidentSvc::IncidentSvc( const std::string& name, ISvcLocator* svc )
  : Service(name, svc),
    m_currentIncidentType(0),
    m_log(msgSvc(), name)
{
}

IncidentSvc::~IncidentSvc() {
  boost::recursive_mutex::scoped_lock lock(m_listenerMapMutex);
  
  for (ListenerMap::iterator i = m_listenerMap.begin();
       i != m_listenerMap.end();
       ++i) {
    delete i->second;
  }
}

//============================================================================================
// Inherited Service overrides:
//============================================================================================
//
StatusCode IncidentSvc::initialize() {
  // initialize the Service Base class
  StatusCode sc = Service::initialize();
  if ( sc.isFailure() ) {
    return sc;
  }

  m_log.setLevel(outputLevel());
  m_currentIncidentType = 0;
   
  // set my own (IncidentSvc) properties via the jobOptionService
  sc = setProperties();
  if ( sc.isFailure() ) {
    m_log << MSG::ERROR << "Could not set my properties" << endreq;
    return sc;
  }

  return StatusCode::SUCCESS;
}

StatusCode IncidentSvc::finalize() {
  // Finalize this specific service
  StatusCode sc = Service::finalize();
  if ( sc.isFailure() ) {
    return sc;
  }

  return StatusCode::SUCCESS;
}

StatusCode IncidentSvc::queryInterface( const InterfaceID& riid, void** ppvInterface ) {
  if ( IID_IIncidentSvc == riid )    {
    *ppvInterface = (IIncidentSvc*)this;
  }
  else  {
    // Interface is not directly available: try out a base class
    return Service::queryInterface(riid, ppvInterface);
  }
  addRef();
  return StatusCode::SUCCESS;
}

//============================================================================================
// Inherited IIncidentSvc overrides:
//============================================================================================
//
void IncidentSvc::addListener(IIncidentListener* lis, const std::string& type,
			      long prio, bool rethrow, bool singleShot) {

  boost::recursive_mutex::scoped_lock lock(m_listenerMapMutex);

  std::string ltype;
  if( type == "" ) ltype = "ALL";
  else             ltype = type;
  // find if the type already exists
  ListenerMap::iterator itMap = m_listenerMap.find( ltype );
  if( itMap == m_listenerMap.end() ) {
    // if not found, create and insert now a list of listeners
    ListenerList* newlist = new ListenerList();
    std::pair<ListenerMap::iterator, bool> p;
    p = m_listenerMap.insert(ListenerMap::value_type(ltype, newlist));
    if( p.second ) itMap = p.first;
  }
  ListenerList* llist = (*itMap).second;
  // add Listener in the ListenerList according to the priority
  ListenerList::iterator itlist;
  for( itlist = llist->begin(); itlist != llist->end(); itlist++ ) {
    if( (*itlist).priority < prio ) {
      // We insert before the current position
      break;
    }
  }

  m_log << MSG::DEBUG << "Adding [" << type << "] listener '" << getListenerName(lis)
        << "' with priority " << prio << endreq;

  llist->insert(itlist, Listener(lis, prio, rethrow, singleShot));
}

void IncidentSvc::removeListener(IIncidentListener* lis, const std::string& type) {

  boost::recursive_mutex::scoped_lock lock(m_listenerMapMutex);
  
  if( type == "") {
    // remove Listener from all the lists
    ListenerMap::iterator itmap;
    for ( itmap = m_listenerMap.begin(); itmap != m_listenerMap.end();) {
      // since the current entry may be eventually deleted
      // we need to keep a memory of the next index before calling recursively this method
      ListenerMap::iterator itmap_old = itmap;
      itmap++;
      removeListener( lis, (*itmap_old).first );
    }
  }
  else {
    ListenerMap::iterator itmap = m_listenerMap.find( type );

    if( itmap == m_listenerMap.end() ) {
      // if not found the incident type then return
      return;
    }
    else {     
      ListenerList* llist = (*itmap).second;
      ListenerList::iterator itlist;
      bool justScheduleForRemoval = ( 0!= m_currentIncidentType )
                                    && (type == *m_currentIncidentType);
      // loop over all the entries in the Listener list to remove all of them than matches
      // the listener address. Remember the next index before erasing the current one
      for( itlist = llist->begin(); itlist != llist->end(); ) {
        if( (*itlist).iListener == lis || lis == 0) {
          if (justScheduleForRemoval) {
            (itlist++)->singleShot = true; // remove it as soon as it is safe
          }
          else {
            m_log << MSG::DEBUG << "Removing [" << type << "] listener '"
                << getListenerName(lis) << "'" << endreq;
            itlist = llist->erase(itlist); // remove from the list now
          }
        }
        else {
          itlist++;
        }
      }
      if( llist->size() == 0) {
        delete llist;
        m_listenerMap.erase(itmap);
      }
    }
  }
}

namespace {
  /// @class listenerToBeRemoved
  /// Helper class to identify a Listener that have to be removed from a list.
  struct listenerToBeRemoved{
    inline bool operator() (const IncidentSvc::Listener& l) {
      return l.singleShot;
    }
  };
}

void IncidentSvc::i_fireIncident( const Incident& incident, const std::string& listenerType ) {

  boost::recursive_mutex::scoped_lock lock(m_listenerMapMutex);
  
  ListenerMap::iterator itmap = m_listenerMap.find( listenerType );
  if ( m_listenerMap.end() == itmap ) return;

  // setting this pointer will avoid that a call to removeListener() during
  // the loop triggers a segfault
  m_currentIncidentType = &(incident.type());

  ListenerList* llist = (*itmap).second;
  ListenerList::iterator itlist;
  bool weHaveToCleanUp = false;
  // loop over all registered Listeners
  for( itlist = llist->begin(); itlist != llist->end(); itlist++ ) {
    m_log << MSG::VERBOSE << "Calling '" << getListenerName((*itlist).iListener)
          << "' for incident [" << incident.type() << "]" << endreq;

    // handle exceptions if they occur
    try {
      (*itlist).iListener->handle(incident);
    }
    catch( const GaudiException& exc ) {
      m_log << MSG::ERROR << "Exception with tag=" << exc.tag() << " is caught " << endreq;
      m_log << MSG::ERROR <<  exc  << endreq;
      if ( (*itlist).rethrow ) { throw (exc); }
    }
    catch( const std::exception& exc ) {
      m_log << MSG::ERROR << "Standard std::exception is caught " << endreq;
      m_log << MSG::ERROR << exc.what()  << endreq;
      if ( (*itlist).rethrow ) { throw (exc); }
    }
    catch(...) {
      m_log << MSG::ERROR << "UNKNOWN Exception is caught " << endreq;
      if ( (*itlist).rethrow ) { throw; }
    }
    // check if at least one of the listeners is a one-shot
    weHaveToCleanUp |= itlist->singleShot;
  }
  if (weHaveToCleanUp) {
    // remove all the listeners that need to be removed from the list
    llist->remove_if( listenerToBeRemoved() );
    // if the list is empty, we can remove it
    if( llist->size() == 0) {
      delete llist;
      m_listenerMap.erase(itmap);
    }
  }

  m_currentIncidentType = 0;
}

void IncidentSvc::fireIncident( const Incident& incident ) {
    
  // Call specific listeners
  i_fireIncident(incident, incident.type());
  // Try listeners registered for ALL incidents
  if ( incident.type() != "ALL" ){ // avoid double calls if somebody fires the incident "ALL"
    i_fireIncident(incident, "ALL");
  }
}