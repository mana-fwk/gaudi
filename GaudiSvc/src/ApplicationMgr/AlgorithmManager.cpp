// $Id: AlgorithmManager.cpp,v 1.8 2007/12/12 16:03:19 marcocle Exp $

// Include files
#include "AlgorithmManager.h"
#include "GaudiKernel/IAlgorithm.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/System.h"
#include "GaudiKernel/MsgStream.h"
#include <iostream>

using ROOT::Reflex::PluginService;

// constructor
AlgorithmManager::AlgorithmManager(IInterface* iface) {
  m_pOuter = iface;
  m_pOuter->queryInterface(IID_ISvcLocator, pp_cast<void>(&m_svclocator)).ignore();
  m_msgsvc     = 0;
  m_refcount   = 1;
  m_listalg    = new ListAlg();
  m_listmgralg = new ListAlg();
}

// destructor
AlgorithmManager::~AlgorithmManager() {
  delete m_listalg;
  delete m_listmgralg;
  if( m_msgsvc ) m_msgsvc->release();
  if( m_svclocator ) m_svclocator->release();
}

// addRef
unsigned long AlgorithmManager::addRef() {
  m_refcount++;
  return m_refcount;
}

// release
unsigned long AlgorithmManager::release() {
  unsigned long count = --m_refcount;
  if( count <= 0) {
    delete this;
  }
  return count;
}

// queryInterface
StatusCode AlgorithmManager::queryInterface(const InterfaceID& iid, void** pinterface) {
  if( iid == IID_IInterface ) {  
    *pinterface = (IInterface*)this;
    addRef();
    return StatusCode::SUCCESS;
  } 
  else if ( iid == IID_IAlgManager ) {
    *pinterface = (IAlgManager*)this;
    addRef();
    return StatusCode::SUCCESS;
  } 
  else {
    return m_pOuter->queryInterface(iid, pinterface);
  }
  return StatusCode::SUCCESS;
}

// addAlgorithm
StatusCode AlgorithmManager::addAlgorithm( IAlgorithm* alg ) {
  m_listalg->push_back( alg );
  return StatusCode::SUCCESS;
}

// removeAlgorithm
StatusCode AlgorithmManager::removeAlgorithm( IAlgorithm* alg ) {
  ListAlg::iterator it;
  for (it = m_listalg->begin(); it != m_listalg->end(); it++ ) {
    if( *it == alg ) {
      m_listalg->erase(it);
      break;
    }
  }
  return StatusCode::SUCCESS;
}

// createService
StatusCode AlgorithmManager::createAlgorithm( const std::string& algtype, 
                                              const std::string& algname,
                                              IAlgorithm*& algorithm,
                                              bool managed) 
{
  // Access the message service if not yet done already
  if( m_msgsvc == 0 ) {
    IMessageSvc **ptr = &m_msgsvc;
    m_svclocator->getService( "MessageSvc", IID_IMessageSvc, (IInterface*&)*ptr ).ignore();
  }
  MsgStream log(m_msgsvc, "AlgorithmManager");
  // Check is the algorithm is already existing
  if( existsAlgorithm( algname ) ) {
    // return an error because an algorithm with that name already exists
    return StatusCode::FAILURE;
  }
  algorithm = PluginService::Create<IAlgorithm*>(algtype, algname, m_svclocator);
  if ( algorithm ) {
    // Check the compatibility of the version of the interface obtained
    if( !isValidInterface(algorithm) ) {
      log << MSG::FATAL << "Incompatible interface IAlgorithm version for " << algtype << endreq;
      return StatusCode::FAILURE;
    }
    StatusCode rc;
    m_listalg->push_back( algorithm );
    if ( managed ) {
      algorithm->addRef();
      m_listmgralg->push_back( algorithm );
      rc = algorithm->sysInitialize();
      if ( !rc.isSuccess() )  {
        log << MSG::ERROR << "Failed to initialize algorithm: "
            << "[" << algname << "]" << endmsg;
      }
    }
    return rc;
  }
  log << MSG::ERROR << "Algorithm of type " << algtype 
      << " is unknown (No factory available)." << endmsg;
  return StatusCode::FAILURE;
}

// getAlgorithm
StatusCode AlgorithmManager::getAlgorithm( const std::string& name, IAlgorithm*& alg) const {
  ListAlg::const_iterator it;
  for (it = m_listalg->begin(); it != m_listalg->end(); it++ ) {
    if( (*it)->name() == name ) {
      alg = *it;
      return StatusCode::SUCCESS;
    }
  }
  return StatusCode::FAILURE;
}

// existsAlgorithm
bool AlgorithmManager::existsAlgorithm( const std::string& name ) const {
  ListAlg::const_iterator it;
  for (it = m_listalg->begin(); it != m_listalg->end(); it++ ) {
	  if( (*it)->name() == name ) {
      return true;
    }
  }
  return false;
}

  // Return the list of Algorithms
std::list<IAlgorithm*>& AlgorithmManager::getAlgorithms( ) const
{
  return *m_listalg;
}

StatusCode AlgorithmManager::initializeAlgorithms() {
  StatusCode rc;
  ListAlg::const_iterator it;
  for (it = m_listmgralg->begin(); it != m_listmgralg->end(); it++ ) {
    rc = (*it)->sysInitialize();
    if ( rc.isFailure() ) return rc; 
  }
  return rc;
}

StatusCode AlgorithmManager::finalizeAlgorithms() {
  StatusCode rc;
  ListAlg::const_iterator it;
  for (it = m_listmgralg->begin(); it != m_listmgralg->end(); it++ ) {
    rc = (*it)->sysFinalize();
    if( rc.isFailure() ) return rc;
    (*it)->release();
  }
  m_listmgralg->clear();
  return rc;
}

