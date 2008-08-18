// $Id: PoolDbCacheSvc.cpp,v 1.19 2008/01/30 13:49:09 marcocle Exp $
//====================================================================
//	PoolDbCacheSvc implementation
//--------------------------------------------------------------------
//	Author     : M.Frank
//
//====================================================================
#include "GaudiPoolDb/PoolDbCacheSvc.h"
#include "GaudiKernel/ClassID.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/strcasecmp.h"
#include "GaudiKernel/SvcFactory.h"

#include "POOLCore/POOLContext.h"

#include "StorageSvc/DbOption.h"
#include "StorageSvc/IDbOptionProxy.h"
#include "StorageSvc/DbInstanceCount.h"
#include "GaudiUtils/IFileCatalog.h"
#include "GaudiUtils/IIODataManager.h"

#include "Reflex/Reflex.h"

namespace GaudiPoolDb  {  bool patchStreamers(MsgStream& log);     }
namespace pool         {  void setLogLevel(seal::Msg::Level lvl);  }

static pool::DbInstanceCount::Counter* s_count = 
  pool::DbInstanceCount::getCounter(typeid(PoolDbCacheSvc));

DECLARE_SERVICE_FACTORY(PoolDbCacheSvc);

namespace {
  class Context {
  public:
    Context() {
      pool::POOLContext::loadComponent( "SEAL/Services/MessageService" );
      setVerbosity( seal::Msg::Info );
    }
    virtual ~Context() {
    }
    static Context& instance()  {
      std::auto_ptr<Context> s_context;
      if ( 0 == s_context.get() ) {
        s_context = std::auto_ptr<Context>(new Context());
      }
      return *(s_context.get());
    }
    void setVerbosity( seal::Msg::Level lvl) {
      pool::POOLContext::setMessageVerbosityLevel( lvl );
    }
  };
}

/// Standard constructor
PoolDbCacheSvc::PoolDbCacheSvc(const std::string& nam, ISvcLocator* svc)
  : Service(nam, svc), m_callbackHandler(0)
{
  s_count->increment();
  Context::instance();
  m_callbackHandler = this;
  declareProperty("Dlls",               m_dlls);
  declareProperty("DomainOpts",         m_domainOpts);
  declareProperty("DatabaseOpts",       m_databaseOpts);
  declareProperty("DatabaseOnOpenOpts", m_databaseOpenOpts);
  declareProperty("ContainerOpts",      m_containerOpts);
  declareProperty("ContainerOnOpenOpts",m_containerOpenOpts);
}

/// Standard destructor
PoolDbCacheSvc::~PoolDbCacheSvc()
{
  s_count->decrement();
}

/// Service overload: Query interface
StatusCode PoolDbCacheSvc::queryInterface(const InterfaceID& riid, void** ppvInterface)
{
  if ( IPoolCacheSvc::interfaceID().versionMatch(riid) )  {
    *ppvInterface = (IPoolCacheSvc*)this;
    addRef();
    return StatusCode::SUCCESS;
  }
  // Interface is not directly availible: try out a base class
  return Service::queryInterface(riid, ppvInterface);
}

/// Service overload: initialize service
StatusCode PoolDbCacheSvc::initialize()  {
  StatusCode status = Service::initialize();
  MsgStream log(messageService(), name());
  if ( !status.isSuccess() ) {
    log << MSG::ERROR << "Failed to initialize Service base class."
        << endmsg;
    return status;
  }
  log << MSG::INFO << "POOL output threshold:" << m_outputLevel
      << endmsg;
  //Context::instance().setVerbosity( static_cast<seal::Msg::Level>(m_outputLevel.value()) );
  pool::setLogLevel(static_cast<seal::Msg::Level>(m_outputLevel.value()));
  status = loadLibraries();
  if ( !status.isSuccess() ) {
    log << MSG::ERROR << "Failed to load POOL libraries."
        << endmsg;
    return status;
  }
  session().open(0);
  // All dictionaries should be loaded. Let's patch the streamers
  // for ContainedObject, DataObject and SmartRefbase
  if ( !GaudiPoolDb::patchStreamers(log) )  {
    log << MSG::ERROR << "Failed to install customized IO!" << endmsg;
    return StatusCode::FAILURE;
  }
  return status;
}

/// Service overload: Finalize service
StatusCode PoolDbCacheSvc::finalize()   {
  MsgStream log(messageService(), name());
  session().close();
  /// Shared Gaudi libraries
  std::vector<System::ImageHandle>::iterator i;
  for(i=m_sharedHdls.begin(); i != m_sharedHdls.end(); ++i)  {
    if ( *i ) {
      // System::unloadDynamicLib(*i);
    }
  }
  m_sharedHdls.clear();
  return Service::finalize();
}

/// Set container specific options
pool::DbStatus 
PoolDbCacheSvc::setCallbackOptions( pool::IDbOptionProxy* pObj,
                                    const std::vector<std::string>& v,
                                    const std::string& obj)
{
  std::string opt_nam, opt_val, opt_typ;
  typedef std::vector<std::string> StringV;
  for (StringV::const_iterator i=v.begin(); i!=v.end(); ++i)  {
    const std::string& id = *i;
    if ( id.length() > obj.length() )   {
      if ( obj == id.substr(0, obj.length()) )  {
        std::string n = id.substr(obj.length()+1);
        size_t idx = n.find("=");
        size_t idx2 = n.find("TYP=");
        if ( idx != std::string::npos && idx2 != std::string::npos )  {
          opt_nam = n.substr(0, idx);
          opt_val = n.substr(idx+1, idx2-2-idx);
          opt_typ = n.substr(idx2+4,1);
          std::stringstream s(opt_val);
          pool::DbOption opt(opt_nam);
          float fval;
          int   ival;
          long long int lval;
          switch(::toupper(opt_typ[0])) {
            case 'I':
              s >> ival;
              opt._setValue(ival);
              pObj->setOption(opt);
              break;
            case 'L':
              s >> lval;
              opt._setValue(lval);
              pObj->setOption(opt);
              break;
            case 'F':
              s >> fval;
              opt._setValue(fval);
              pObj->setOption(opt);
              break;
            case 'S':
              opt._setValue(opt_val.c_str());
              pObj->setOption(opt);
              break;
            default:
              break;
          }
        }
      }
    }
  }
  return pool::Success;
}

/// Default callback (does nothing)
pool::DbStatus 
PoolDbCacheSvc::setMyOptions( pool::IDbOptionProxy* pObj, 
                              pool::DbOptionCallback::OptionType typ, 
                              const std::string& name)
{
  switch (typ)  {
    case pool::DbOptionCallback::_DOMAIN_OPT:
      return setCallbackOptions(pObj, m_domainOpts, name);
    case pool::DbOptionCallback::_DATABASE_OPT:
      return setCallbackOptions(pObj, m_databaseOpts, name);
    case pool::DbOptionCallback::_DATABASE_ONOPEN:
      return setCallbackOptions(pObj, m_databaseOpenOpts, name);
    case pool::DbOptionCallback::_CONTAINER_OPT:
      return setCallbackOptions(pObj, m_containerOpts, name);
    case pool::DbOptionCallback::_CONTAINER_ONOPEN:
      return setCallbackOptions(pObj, m_containerOpenOpts, name);
    default:
      break;
  }
  return pool::Error;
}

// Load all required libraries
StatusCode PoolDbCacheSvc::loadLibraries()  {
  StatusCode status = StatusCode::SUCCESS;
  if ( !m_dlls.empty() )  {
    std::vector<std::string>::const_iterator i;
    for(i=m_dlls.begin(); i != m_dlls.end(); ++i)  {
      StatusCode iret = loadDictionary(*i);
      if ( !iret.isSuccess() )  {
        status = iret;
      }
    }
  }
  return status;
}

/// Load dictionary library
StatusCode PoolDbCacheSvc::loadDictionary(const std::string& nam) {
  System::ImageHandle hdl = 0;
  StatusCode status = System::loadDynamicLib(nam, &hdl);
  if ( !status.isSuccess() )  {
    MsgStream log(messageService(), name());
    log << MSG::ERROR << System::getLastErrorString() << endmsg
        << "Failed to load POOL implementation library:"
        << nam << endmsg;
    return status;
  }
  m_sharedHdls.push_back(hdl);
  return status;
}