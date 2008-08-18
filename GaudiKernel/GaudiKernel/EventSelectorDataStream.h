//====================================================================
//	EventSelectorDataStream.h
//--------------------------------------------------------------------
//
//	Package    : EventSelectorDataStream  (LHCb Event Selector Package)
//
//	Author     : M.Frank
//  Created    : 4/10/00
//
//====================================================================
#ifndef GAUDIKERNEL_EVENTSELECTORDATASTREAM_H
#define GAUDIKERNEL_EVENTSELECTORDATASTREAM_H 1

// Include files
#include "GaudiKernel/IEvtSelector.h"
#include "GaudiKernel/Property.h"

// STL include files
#include <vector>
#include <iostream>

// Forward declarations
class MsgStream;
class ISvcLocator;
class EventSelectorDataStream;

/** Definition of class EventSelectorDataStream

    Small class which eases the management of multiple 
    input streams for the event selector.

    History:
    +---------+----------------------------------------------+---------+
    |    Date |                 Comment                      | Who     |
    +---------+----------------------------------------------+---------+
    | 3/10/00 | Initial version                              | M.Frank |
    +---------+----------------------------------------------+---------+
   @author Markus Frank
   @version 1.0
*/
class EventSelectorDataStream : virtual public IInterface   {
  /// Output friend for MsgStream
  friend MsgStream& operator<<(MsgStream& s, const EventSelectorDataStream& obj);
  /// Output friend for standard I/O
  friend std::ostream& operator<<(std::ostream& s, const EventSelectorDataStream& obj);
public:
  typedef std::vector< StringProperty >  Properties;
protected:
  /// Reference count
  long                    m_refCount;
  /// Name
  std::string             m_name;
  /// Definition string
  std::string             m_definition;
  /// Criteria
  std::string             m_criteria;
  /// Event selector type
  std::string             m_selectorType;
  /// Pointer to valid selector
  IEvtSelector*           m_pSelector;
  /// Reference to service locator
  ISvcLocator*            m_pSvcLocator;
  /// Properties vector
  Properties*             m_properties;
  /// Initialization state
  bool                    m_initialized;
  /// Standard Destructor
  virtual ~EventSelectorDataStream();
public:
  /// Standard Constructor
  EventSelectorDataStream(const std::string& nam, const std::string& def, ISvcLocator* svcloc);
  /** Query interfaces of Interface
      @param riid       ID of Interface to be retrieved
      @param ppvUnknown Pointer to Location for interface pointer
  */
  virtual StatusCode queryInterface(const InterfaceID& riid, void** ppvUnknown);
  /// Reference Interface instance               
  virtual unsigned long addRef();
  /// Release Interface instance                 
  virtual unsigned long release();
  /// Attach event selector object
  virtual void setSelector(IEvtSelector* pSelector);
  /// Parse input criteria
  virtual StatusCode initialize();
  /// Finalize stream and release resources
  virtual StatusCode finalize();
  /// Allow access to individual properties by name
  StringProperty* property(const std::string& nam);
  /// Allow access to individual properties by name (CONST)
  const StringProperty* property(const std::string& nam)  const;
  /// Access properties
  const Properties& properties()    {
    return *m_properties;
  }
  /// Retrieve stream name
  const std::string& name()   const   {
    return m_name;
  }
  /// Retrieve stream criteria
  const std::string& criteria()   const   {
    return m_criteria;
  }
  /// Retrieve event selector type
  const std::string& selectorType()   const   {
    return m_selectorType;
  }
  /// Retrieve definition string
  const std::string& definition() const   {
    return m_definition;
  }
  /// Retrieve event selector object
  IEvtSelector* selector()  const  {
    return m_pSelector;
  }
  /// Check initialisation status
  bool isInitialized()  const   {
    return m_initialized;
  }
};
#endif  // GAUDIKERNEL_EVENTSELECTORDATASTREAM_H