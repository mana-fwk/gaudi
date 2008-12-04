// $Id: AlgContextSvc.h,v 1.4 2007/05/24 13:49:47 hmd Exp $ 
// ============================================================================
// CVS tag $Name:  $, version $Revision: 1.4 $
// ============================================================================
// $Log: AlgContextSvc.h,v $
// Revision 1.4  2007/05/24 13:49:47  hmd
// ( Vanya Belyaev) patch #1171. The enhancement of existing Algorithm Context Service
//    is the primary goal of the proposed patch. The existing
//    AlgContextSvc is not safe with respect to e.g. Data-On-Demand
//    service or to operations with subalgorithms. The patched service
//    essentially implements the queue of executing algorithms, thus the
//    problems are eliminiated. In addition the enriched interface
//    provides the access to the whole queue of executing algorithms.
// 
// ============================================================================
#ifndef GAUDISVC_ALGCONTEXTSVC_H 
#define GAUDISVC_ALGCONTEXTSVC_H 1
// ============================================================================
// Include files 
// ============================================================================
// GaudiKernel
// ============================================================================
#include "GaudiKernel/StatusCode.h"
#include "GaudiKernel/IAlgContextSvc.h"
#include "GaudiKernel/IAlgorithm.h"
#include "GaudiKernel/IIncidentListener.h"
#include "GaudiKernel/Service.h"
// ============================================================================
// Forward declarations 
// ============================================================================
template <class TYPE> class SvcFactory;
class IIncidentSvc ;
// ============================================================================
/** @class AlgContexSvc
 *  Simple implementation of interface IAlgContextSvc
 *  for Algorithm Contetx Service
 *  @author ATLAS Collaboration
 *  @author modified by Vanya BELYAEV ibelyaev@physics.sye.edu
 *  @date 2007-03-07 (modified)
 */
class AlgContextSvc 
  :         public        Service
  , virtual public IAlgContextSvc
  , virtual public IIncidentListener 
{
public:
  /// friend factory for instantiations 
  friend class SvcFactory<AlgContextSvc>;
public:
  /// set     the currently executing algorithm  ("push_back") @see IAlgContextSvc
  virtual StatusCode     setCurrentAlg  ( IAlgorithm* a ) ;
  /// remove the algorithm                       ("pop_back") @see IAlgContextSvc
  virtual StatusCode   unSetCurrentAlg  ( IAlgorithm* a ) ;
  /// accessor to current algorithm: @see IAlgContextSvc
  virtual IAlgorithm*       currentAlg  () const ;
  /// get the stack of executed algorithms @see IAlgContextSvc
  virtual const IAlgContextSvc::Algorithms& algorithms  () const 
  { return m_algorithms ; }
public:
  /// Gaudi_cast, @see IInterface
  virtual StatusCode queryInterface
  ( const InterfaceID& riid , 
    void**             ppvi ) ;
public:
  /// handle incident @see IIncidentListener
  virtual void handle ( const Incident& ) ;
public:
  /// standard initialization of the service @see IService 
  virtual StatusCode initialize () ;
  /// standard finalization  of the service  @see IService
  virtual StatusCode finalize   () ;  
public:
  /// Standard Constructor @see Service
  AlgContextSvc 
  ( const std::string& name , 
    ISvcLocator*       svc  ) ;
  /// Standard Destructor
  virtual ~AlgContextSvc();  
private:
  // default constructor is disabled 
  AlgContextSvc () ; ///< no default constructor   
  // copy constructor is disabled 
  AlgContextSvc ( const AlgContextSvc& ); ///< no copy constructor 
  // assignement operator is disabled 
  AlgContextSvc& operator=( const AlgContextSvc& ); ///< no assignement 
private:
  // the stack of currect algorithms 
  IAlgContextSvc::Algorithms m_algorithms ; ///< the stack of currect algorithms 
  // pointer to Incident Service 
  IIncidentSvc*              m_inc        ; ///< pointer to Incident Service
  // flag to perform more checking
  bool                       m_check      ;
} ;

// ============================================================================
// The END 
// ============================================================================
#endif // GAUDISVC_ALGCONTEXTSVC_H
// ============================================================================
