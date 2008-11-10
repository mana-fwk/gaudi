// $Id: DataOnDemandSvc.cpp,v 1.16 2008/10/01 14:33:07 marcocle Exp $
// ============================================================================
// CVS tag $Name:  $ 
// ============================================================================
// Incldue files 
// ============================================================================
// STD & STL 
// ============================================================================
#include <string>
#include <set>
#include <map>
#include <math.h>
// ============================================================================
// GaudiKernel
// ============================================================================
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/Tokenizer.h"
#include "GaudiKernel/DataObject.h"
#include "GaudiKernel/IAlgorithm.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/IAlgManager.h"
#include "GaudiKernel/IIncidentSvc.h"
#include "GaudiKernel/DataIncident.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/ListItem.h"
#include "GaudiKernel/ToStream.h"
#include "DataOnDemandSvc.h"
// ============================================================================
// Boost 
// ============================================================================
#include "boost/format.hpp"
#include "boost/lexical_cast.hpp"
// ============================================================================

// ============================================================================
// Constructors and Destructor
// ============================================================================
DataOnDemandSvc::DataOnDemandSvc 
( const std::string& name, ISvcLocator* svc )
  : Service(name, svc)
  , m_incSvc   ( 0 )
  , m_algMgr   ( 0 )
  , m_dataSvc  ( 0 )
  //
  , m_trapType    ( "DataFault")
  , m_dataSvcName ( "EventDataSvc" )
  , m_partialPath ( true  ) 
  , m_dump        ( false )
  //
  , m_algMapping  () 
  , m_nodeMapping () 
  //
  , m_algMap   (   ) 
  , m_nodeMap  (   )  
  //
  , m_updateRequired ( true )
  , m_prefix         ( "/Event/" )
  , m_log      ( 0 )
  , m_total    (   ) 
  , m_statAlg  ( 0 ) 
  , m_statNode ( 0 ) 
  , m_stat     ( 0 )
{
  // ==========================================================================
  declareProperty ( "IncidentName"       , m_trapType    ) ;
  declareProperty ( "DataSvc"            , m_dataSvcName ) ;
  declareProperty ( "UsePreceedingPath"  , m_partialPath ) ;
  declareProperty ( "Dump"               , m_dump        ) ;
  //
  declareProperty ( "Algorithms"         , m_algMapping  )  ->
    declareUpdateHandler ( &DataOnDemandSvc::update_2 , this ) ;
  declareProperty ( "Nodes"              , m_nodeMapping ) ->
    declareUpdateHandler ( &DataOnDemandSvc::update_3 , this ) ;
  //
  declareProperty ( "AlgMap"  , m_algMap  ) ->  
    declareUpdateHandler ( &DataOnDemandSvc::update_1 , this ) ;
  declareProperty ( "NodeMap" , m_nodeMap ) ->  
    declareUpdateHandler ( &DataOnDemandSvc::update_1 , this ) ;
  //
  declareProperty ( "Prefix"             , m_prefix      ) ;
  // ==========================================================================
} 
// ============================================================================
// Update handler 
// ============================================================================
void DataOnDemandSvc::update_1 ( Property& p ) 
{
  stream() << MSG::VERBOSE << " I am update handler for property " << p << endreq ;
  // force update 
  m_updateRequired = true ;
} 
// ============================================================================
// Update handler 
// ============================================================================
void DataOnDemandSvc::update_3 ( Property& /* p */ ) 
{
  stream() << MSG::WARNING 
           << "The property 'Nodes'      is obsolete, switch to map-like 'NodeMap' "
           << " = { 'data' : 'type'      } "
           << endreq ;
  // force update 
  m_updateRequired = true ;
} 
// ============================================================================
// Update handler 
// ============================================================================
void DataOnDemandSvc::update_2 ( Property& /* p */ ) 
{
  stream() << MSG::WARNING 
           << "The property 'Algorithms' is obsolete, switch to map-like 'AlgMap'  " 
           << " = { 'data' : 'algorithm' } "
           << endreq ;
  // force update 
  m_updateRequired = true ;
}
// ============================================================================
// anonymous namespace to hide few local functions 
// ============================================================================
namespace 
{
  // ==========================================================================
  /** remove the prefix from the string
   *  @param value input tring
   *  @param prefix prefix to be removed 
   *  @return input string without prefix 
   */
  inline std::string no_prefix 
  ( const std::string& value  , 
    const std::string& prefix )
  {
    return 
      !prefix.empty() && 0 == value.find(prefix) ? 
      std::string( value , prefix.size() ) : value ;
  }
  // ==========================================================================
  /** add a prefix (if needed) to all keys of the map
   *  the previus key is removed
   *  @param _map map to be modified 
   *  @param prefix prefix to be added 
   *  @return number of modified keys 
   */
  template <class MAP>
  inline size_t add_prefix ( MAP& _map , const std::string& prefix ) 
  {
    // empty  prefix 
    if ( prefix.empty() ) { return 0 ; }                    // RETURN 
    /// loop over all entries to find the  proper keys 
    for ( typename MAP::iterator it = _map.begin() ; _map.end() != it ; ++it ) 
    {
      if ( 0 != it->first.find(prefix) )  // valid prefix? 
      { 
        std::string key   = prefix + it->first ;
        std::string value = it->second ;
        _map.erase ( it ) ;
        _map[ key ] = value  ;
        return 1 + add_prefix ( _map , prefix ) ;    // RETURN, recursion
      }
    }
    //
    return 0 ;
  }
  // ==========================================================================
  /** get the list of directories for a certain TES location
   *  @param object to be inspected
   *  @param _set (output) the set with directoiry list
   *  @return incrment of the output set size 
   */
  template <class SET>
  inline size_t get_dir ( const std::string& object , SET& _set ) 
  {
    std::string::size_type ifind = object.rfind('/') ;
    // stop recursion 
    if ( std::string::npos == ifind ) { return 0 ; } // RETURN 
    if ( 0 == ifind                 ) { return 0 ; }
    //
    const std::string top = std::string( object , 0 , ifind) ;
    _set.insert( top ) ;
    return 1 + get_dir ( top , _set ) ;   // RETURN, recursion 
  }
  // ==========================================================================
  /** get the list of directories for a all keys of the input map
   *  @param _map (input) map to be inspected 
   *  @param _set (output) the set with directoiry list
   *  @return incrment of the output set size 
   */
  template <class MAP, class SET>
  inline size_t get_dirs ( const MAP& _map, SET& _set ) 
  {
    size_t size = _set.size() ;
    for ( typename MAP::const_iterator item = _map.begin() ; 
          _map.end() != item ; ++item ) {  get_dir ( item->first , _set ) ; }
    return _set.size() - size ;
  } 
  // ==========================================================================
} // end of anonymous namespace
// ============================================================================
// update the content of Data-On-Demand actions 
// ============================================================================
StatusCode DataOnDemandSvc::update () 
{
  StatusCode sc = StatusCode::SUCCESS ;
  /// convert obsolete "Nodes"      into new "NodeMap"
  sc = setupNodeHandlers() ; // convert "Nodes"      new "NodeMap"
  if ( sc.isFailure() ) 
  {
    stream() << MSG::ERROR << "Failed to setup old \"Nodes\""      << endreq ;
    return sc ;
  }
  /// convert obsolete "Algorithms" into new "AlgMap"
  sc = setupAlgHandlers()   ; // convert "Algorithms" into "AlgMap"
  if ( sc.isFailure() ) 
  {
    stream() << MSG::ERROR << "Failed to setup old \"Algorithms\"" << endreq ;
    return sc ;
  }
  /// add the default prefix 
  add_prefix ( m_algMap  , m_prefix ) ;
  /// add the default prefix 
  add_prefix ( m_nodeMap , m_prefix ) ;
  /// get all directories 
  typedef std::set<std::string> Set ; 
  Set dirs ;
  if ( m_partialPath ){ get_dirs ( m_algMap  , dirs ) ; }
  if ( m_partialPath ){ get_dirs ( m_nodeMap , dirs ) ; }
  //
  Set::iterator _e = dirs.find("/Event") ;
  if ( dirs.end() != _e ) { dirs.erase( _e ) ; }
  // add all directories as nodes 
  for ( Set::const_iterator dir = dirs.begin() ; dirs.end() != dir ; ++dir ) 
  {
    if ( m_algMap  .end () != m_algMap  .find ( *dir ) ) { continue ; }
    if ( m_nodeMap .end () != m_nodeMap .find ( *dir ) ) { continue ; }    
    m_nodeMap [*dir] = "DataObject" ;  
  }
  //
  m_algs  .clear  () ;
  m_nodes .clear  () ;
  //
  /// setup algorithms
  for ( Map::const_iterator ialg = m_algMap.begin() ; 
        m_algMap.end() != ialg ; ++ialg )
  {
    ListItem alg ( ialg->second ) ;
    m_algs[ialg->first] =  Leaf( alg.type() , alg.name() ) ;
  }
  /// setup nodes 
  for ( Map::const_iterator inode = m_nodeMap.begin() ; 
        m_nodeMap.end() != inode ; ++inode ) 
  {
    ClassH cl = ROOT::Reflex::Type::ByName( inode->second ) ;
    if ( !cl ) 
    {
      stream() << MSG::WARNING 
               << "Failed to access dictionary class for "
               << inode->first << " of type:" << inode->second << endmsg;
    }
    m_nodes[inode->first] = Node ( cl , false , inode->second ) ;
  }
  ///
  m_updateRequired = false ;
  //
  return StatusCode::SUCCESS ;
}
// ============================================================================
// destructor 
// ============================================================================
DataOnDemandSvc::~DataOnDemandSvc() 
{ if ( 0 != m_log      ) { delete m_log      ; m_log      = 0 ; } }
//=============================================================================
// Inherited Service overrides:
//=============================================================================
StatusCode DataOnDemandSvc::initialize() 
{
  // initialize the Service Base class
  StatusCode sc = Service::initialize();
  if ( sc.isFailure() )  { return sc; }
  sc = setup();
  if ( sc.isFailure() )  { return sc; }
  //
  if      ( m_dump )                      { dump ( MSG::INFO  ) ; }
  else if ( MSG::DEBUG >= outputLevel() ) { dump ( MSG::DEBUG ) ; }
  //
  return StatusCode::SUCCESS ;
}
// ============================================================================
/* dump the content of DataOnDemand service 
 *  @param level the printout level
 *  @param mode  the printout mode 
 */
// ============================================================================
void DataOnDemandSvc::dump
( const MSG::Level level , 
  const bool       mode  )  const 
{
  if ( m_algs.empty()  &&  m_nodes.empty() ) { return ; }

  typedef std::pair<std::string,std::string> Pair ;
  typedef std::map<std::string,Pair>         PMap ;
  
  PMap _m ;
  for ( AlgMap::const_iterator alg = m_algs.begin() ; 
        m_algs.end() != alg ; ++alg )
  {
    PMap::const_iterator check = _m.find(alg->first) ;
    if ( _m.end() != check ) 
    { 
      stream() 
        << MSG::WARNING 
        << " The data item is activated for '"
        << check->first << "' as '" << check->second.first << "'" << endreq ;
    }
    const Leaf& l = alg->second ;
    std::string nam = ( l.name == l.type ? l.type  : (l.type+"/"+l.name) ) ;
    //
    if ( !mode && 0 == l.num ) { continue ; }
    //
    std::string val ;
    if ( mode ) { val = ( 0 == l.algorithm ) ? "F" : "T" ; }
    else { val = boost::lexical_cast<std::string>( l.num ) ; }
    //
    _m[ no_prefix ( alg->first , m_prefix ) ] = std::make_pair ( nam , val ) ;
  }
  // nodes:
  for ( NodeMap::const_iterator node = m_nodes.begin() ; 
        m_nodes.end() != node ; ++node )
  {
    PMap::const_iterator check = _m.find(node->first) ;
    if ( _m.end() != check ) 
    {
      stream()
        << MSG::WARNING 
        << " The data item is already activated for '"
        << check->first << "' as '" << check->second.first << "'" << endreq ;
    }
    const Node& n = node->second ;
    std::string nam = "'" + n.name + "'"  ; 
    //
    std::string val ;

    if ( !mode && 0 == n.num ) { continue ; }
    
    if ( mode ) { val = ( 0 == n.clazz ) ? "F" : "T" ; }
    else { val = boost::lexical_cast<std::string>( n.num ) ; }    
    //
    _m[ no_prefix ( node->first , m_prefix ) ] = std::make_pair ( nam , val ) ;
  }
  //
  if ( _m.empty() ) { return ; }
  
  // find the correct formats 
  size_t n1 = 0 ;
  size_t n2 = 0 ;
  size_t n3 = 0 ;
  for  ( PMap::const_iterator it = _m.begin() ; _m.end() != it ; ++it ) 
  {
    n1 = std::max ( n1 , it->first.size()         ) ;
    n2 = std::max ( n2 , it->second.first.size()  ) ;    
    n3 = std::max ( n3 , it->second.second.size() ) ;
  }
  if ( 10 > n1 ) { n1 = 10 ; } 
  if ( 10 > n2 ) { n2 = 10 ; } 
  if ( 60 < n1 ) { n1 = 60 ; } 
  if ( 60 < n2 ) { n2 = 60 ; } 
  //
  
  const std::string _f = " | %%1$-%1%.%1%s | %%2$-%2%.%2%s | %%3$%3%.%3%s |" ;
  boost::format _ff ( _f ) ;
  _ff % n1 % n2 % n3 ;
  
  const std::string _format  = _ff.str() ;
  
  MsgStream& msg = stream() << level ;
  
  if ( mode ) { msg << "Data-On-Demand Actions enabled for:"       ; }
  else        { msg << "Data-On-Demand Actions has been used for:" ; }
  
  boost::format fmt1( _format)  ;
  fmt1 % "Address" % "Creator" % ( mode ? "S" : "#" ) ;
  //
  const std::string header = fmt1.str() ;
  std::string line = std::string( header.size() , '-' ) ;
  line[0] = ' ' ;
  
  msg << std::endl << line 
      << std::endl << header 
      << std::endl << line ;
  
  // make the actual printout:
  for ( PMap::const_iterator item = _m.begin() ; 
        _m.end() != item ; ++item ) 
  {    
    boost::format fmt( _format)  ;
    msg << std::endl << 
      ( fmt % item->first % item->second.first % item->second.second ) ;
  }
  
  msg << std::endl << line << endreq ;

}  
// ============================================================================
// finalization of the service 
// ============================================================================
StatusCode DataOnDemandSvc::finalize() 
{
  //
  stream ()
    << MSG::INFO 
    << "Handled \"" << m_trapType << "\" incidents: "
    << m_statAlg  << "/" << m_statNode << "/" << m_stat << "(Alg/Node/Total)."
    << endreq ;
  if ( m_dump || MSG::DEBUG >= outputLevel() ) 
  {
    stream ()
      << MSG::INFO 
      << m_total.outputUserTime 
      ( "Algorithm timing: Mean(+-rms)/Min/Max:%3%(+-%4%)/%6%/%7%[ms] " , System::milliSec ) 
      << m_total.outputUserTime ( "Total:%2%[s]" , System::Sec ) << endreq ;  
  }
  // dump it! 
  if      ( m_dump )                      { dump ( MSG::INFO  , false ) ; }
  else if ( MSG::DEBUG >= outputLevel() ) { dump ( MSG::DEBUG , false ) ; }
  //
  if ( m_incSvc )  
  {
    m_incSvc->removeListener(this, m_trapType);
    m_incSvc->release();
    m_incSvc = 0;
  }
  if ( 0 != m_algMgr  ) { m_algMgr   -> release () ; m_algMgr  = 0 ; }
  if ( 0 != m_dataSvc ) { m_dataSvc  -> release () ; m_dataSvc = 0 ; }
  //
  return Service::finalize();
}
// ============================================================================
/// re-initialization of the service 
// ============================================================================
StatusCode DataOnDemandSvc::reinitialize() 
{
  // reinitialize the Service Base class
  if ( 0 != m_incSvc )  
  {
    m_incSvc -> removeListener ( this , m_trapType );
    m_incSvc -> release ();
    m_incSvc = 0;
  }
  if ( 0 != m_algMgr  ) { m_algMgr   -> release() ; m_algMgr  = 0 ; }
  if ( 0 != m_dataSvc ) { m_dataSvc  -> release() ; m_dataSvc = 0 ; }
  if ( 0 != m_log     ) { delete m_log ; m_log = 0 ; }
  //
  StatusCode sc = Service::reinitialize();
  if ( sc.isFailure() )  { return sc; }
  //
  sc = setup() ;
  if ( sc.isFailure() )  { return sc; }
  //
  if ( m_dump ) { dump ( MSG::INFO ) ; }
  else if ( MSG::DEBUG >= outputLevel() ) { dump ( MSG::DEBUG  ) ; }
  //
  return StatusCode::SUCCESS ;
}
// ============================================================================
// query interface 
// ============================================================================
StatusCode DataOnDemandSvc::queryInterface
( const InterfaceID& riid, 
  void** ppvInterface ) 
{
  if ( IIncidentListener::interfaceID() == riid )    
  {
    *ppvInterface = static_cast<IIncidentListener*>(this);
    addRef();
    return StatusCode::SUCCESS;
  }
  // Interface is not directly available: try out a base class
  return Service::queryInterface(riid, ppvInterface);
}
// ============================================================================
// setup service 
// ============================================================================
StatusCode DataOnDemandSvc::setup() 
{
  m_algMgr = 0;
  StatusCode sc = 
    serviceLocator()->queryInterface(IID_IAlgManager, pp_cast<void>(&m_algMgr));
  if ( sc.isFailure () )  
  {
    stream() 
      << MSG::ERROR
      << "Failed to retrieve the IAlgManager interface." << endmsg;
    return sc;
  }
  sc = service("IncidentSvc", m_incSvc, true);
  if ( sc.isFailure () )  
  {
    stream() 
      << MSG::ERROR << "Failed to retrieve Incident service." << endmsg;
    return sc;
  }
  m_incSvc->addListener(this, m_trapType);
  sc = service(m_dataSvcName, m_dataSvc, true);
  if ( sc.isFailure () )  
  {
    stream() 
      << MSG::ERROR
      << "Failed to retrieve the data provider interface of "
      << m_dataSvcName << endmsg;
    return sc;
  }
  return update() ;
}
// ============================================================================
// setup node handlers 
// ============================================================================
StatusCode DataOnDemandSvc::setupNodeHandlers()  
{
  Setup::const_iterator j;
  std::string nam, typ, tag;
  StatusCode sc = StatusCode::SUCCESS;
  // Setup for node leafs, where simply a constructor is called...
  for ( j=m_nodeMapping.begin(); j != m_nodeMapping.end(); ++j) 
  {
    Tokenizer tok(true);
    tok.analyse(*j, " ", "", "", "=", "'", "'");
    for ( Tokenizer::Items::iterator i = tok.items().begin(); 
          i != tok.items().end(); i++ )   {
      const std::string& t = (*i).tag();
      const std::string& v = (*i).value();
      switch( ::toupper(t[0]) )    {
      case 'D':
        tag = v;
        break;
      case 'T':
        nam = v;
        break;
      }
    } 
    if ( m_algMap  .end () != m_algMap  .find ( tag ) || 
         m_nodeMap .end () != m_nodeMap .find ( tag ) )
    {
      stream() 
        << MSG::WARNING 
        << "The obsolete property 'Nodes' redefines the action for '" 
        + tag + "' to be '" +nam+"'"
        << endreq ;
    }
    m_nodeMap[tag] = nam ; 
  }
  //
  m_updateRequired = true ;
  //
  return sc;
}
// ============================================================================
// setup algorithm  handlers 
// ============================================================================
StatusCode DataOnDemandSvc::setupAlgHandlers()  
{
  Setup::const_iterator j;
  std::string typ, tag;
  
  for(j=m_algMapping.begin(); j != m_algMapping.end(); ++j)  
  {
    Tokenizer tok(true);
    tok.analyse(*j, " ", "", "", "=", "'", "'");
    for(Tokenizer::Items::iterator i = tok.items().begin(); i != tok.items().end(); i++ )   {
      const std::string& t = (*i).tag();
      const std::string& v = (*i).value();
      switch( ::toupper(t[0]) )    {
      case 'D':
        tag = v;
        break;
      case 'T':
        typ = v;
        break;
      }
    }
    ListItem item(typ);
    if ( m_algMap  .end () != m_algMap  .find ( tag ) || 
         m_nodeMap .end () != m_nodeMap .find ( tag ) )
    {
      stream() 
        << MSG::WARNING 
        << "The obsolete property 'Algorithms' redefines the action for '" 
        + tag + "' to be '" +item.type() +"/"+item.name()+"'"
        << endreq ;
    }
    m_algMap[tag] = item.type() + "/" + item.name() ;
  }
  m_updateRequired = true ;
  return StatusCode::SUCCESS;
}
// ============================================================================
/// Configure handler for leaf
// ============================================================================
StatusCode DataOnDemandSvc::configureHandler(Leaf& l)  
{
  if ( 0 == m_algMgr  ) { return StatusCode::FAILURE ; }
  StatusCode sc = m_algMgr->getAlgorithm(l.name, l.algorithm);
  if ( sc.isFailure() ) 
  {
    sc = m_algMgr->createAlgorithm(l.type, l.name, l.algorithm, true );
    if ( sc.isFailure() ) 
    {
      stream()  
        << MSG::ERROR 
        << "Failed to create algorithm "
        << l.type << "('" << l.name<< "')" << endmsg; 
    }
  }
  return sc;
}
// ===========================================================================
/// IIncidentListener interfaces overrides: incident handling
// ===========================================================================
void DataOnDemandSvc::handle ( const Incident& incident )
{ 
  ++m_stat ;
  // proper incident type? 
  if ( incident.type() != m_trapType ) { return ; }             // RETURN
  const DataIncident* inc = dynamic_cast<const DataIncident*>(&incident);
  if ( 0 == inc                      ) { return ; }             // RETURN
  // update if needed! 
  if ( m_updateRequired ) { update() ; }
  const std::string& tag = inc->tag();
  if ( MSG::VERBOSE >= outputLevel() ) 
  {
    stream()
      << MSG::VERBOSE 
      << "Incident: [" << incident.type   () << "] " 
      << " = "         << incident.source ()
      << " Location:"  << tag  << endmsg;
  }
  // ==========================================================================
  NodeMap::iterator icl = m_nodes.find ( tag ) ;
  if ( icl != m_nodes.end() )  
  {
    StatusCode sc = execHandler ( tag , icl->second ) ;
    if ( sc.isSuccess() ) { ++m_statNode ; } 
    return ;                                                        // RETURN 
  }
  // ==========================================================================
  AlgMap::iterator ialg = m_algs.find ( tag ) ;
  if ( ialg != m_algs.end() )  
  {
    StatusCode sc = execHandler ( tag , ialg->second ) ;
    if ( sc.isSuccess() ) { ++m_statAlg ; }
    return ;                                                        // RETURN 
  }
}
// ===========================================================================
// ecxecute the handler 
// ===========================================================================
StatusCode 
DataOnDemandSvc::execHandler(const std::string& tag, Node& n)  
{
  if ( n.executing ) { return StatusCode::FAILURE ; }            // RETURN 
 
  // try to recover the handler 
  if ( !n.clazz  ) { n.clazz = ROOT::Reflex::Type::ByName(n.name) ; }
  if ( !n.clazz  ) 
  {
    stream() 
      << MSG::ERROR 
      << "Failed to get dictionary for class '"
      << n.name  
      << "' for location:" << tag << endmsg;
    return StatusCode::FAILURE ;                               // RETURN 
  }
  
  ROOT::Reflex::Object obj = n.clazz.Construct();
  DataObject* pO = (DataObject*)obj.Address();
  if ( !pO )
  {
    stream() 
      << MSG::ERROR 
      << "Failed to create an object of type:"
      << n.clazz.Name(ROOT::Reflex::SCOPED) << " for location:" << tag 
      << endmsg;  
    return StatusCode::FAILURE  ;                               // RETURN 
  }
  //
  Protection p(n.executing);
  StatusCode sc = m_dataSvc->registerObject(tag, pO);
  if ( sc.isFailure() ) 
  {
    stream() 
      << MSG::ERROR << "Failed to register an object of type:"
      << n.clazz.Name(ROOT::Reflex::SCOPED) << " at location:" << tag 
      << endmsg;
    return sc ;                                                  // RETURN 
  }
  ++n.num ;
  //
  return StatusCode::SUCCESS ;
}
// ===========================================================================
// execute the handler 
// ===========================================================================
StatusCode 
DataOnDemandSvc::execHandler(const std::string& tag, Leaf& l)  
{
  //
  if ( l.executing ) { return StatusCode::FAILURE ; }             // RETURN  
  //
  if ( 0 == l.algorithm ) 
  {
    StatusCode sc = configureHandler ( l ) ;
    if ( sc.isFailure() ) 
    {
      stream()  
        << MSG::ERROR 
        << "Failed to configure handler for: "
        << l.name << "[" << l.type << "] " << tag << endmsg; 
      return sc ;                                                 // RETURN 
    }
  }
  //
  Timer timer ( m_total ) ;
  //
  Protection p(l.executing);
  StatusCode sc = l.algorithm->sysExecute();
  if ( sc.isFailure() )  
  {
    stream() << MSG::ERROR 
             << "Failed to execute the algorithm:"
             << l.algorithm->name() << " for location:" << tag << endmsg;
    return sc ;                                                       // RETURN 
  }
  ++l.num ;
  //
  return StatusCode::SUCCESS ;
}
// ============================================================================
/** Instantiation of a static factory class used by clients to create
 *  the instances of this service
 */
DECLARE_SERVICE_FACTORY(DataOnDemandSvc)
// ============================================================================
  
// ============================================================================
// The END 
// ============================================================================
