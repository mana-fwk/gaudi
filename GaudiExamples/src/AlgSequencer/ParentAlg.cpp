// $Id: ParentAlg.cpp,v 1.5 2006/11/30 10:35:26 mato Exp $

// Include files
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/DataObject.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "ParentAlg.h"


// Static Factory declaration

DECLARE_ALGORITHM_FACTORY(ParentAlg);

// Constructor
//------------------------------------------------------------------------------
ParentAlg::ParentAlg(const std::string& name, ISvcLocator* ploc)
          : GaudiAlgorithm(name, ploc) {
//------------------------------------------------------------------------------
}

//------------------------------------------------------------------------------
StatusCode ParentAlg::initialize() {
//------------------------------------------------------------------------------
  MsgStream log(msgSvc(), name());
  StatusCode sc;

  log << MSG::INFO << "creating sub-algorithms...." << endreq;

  sc =  createSubAlgorithm( "SubAlg", "SubAlg1", m_subalg1);
  if( sc.isFailure() ) return Error("Error creating Sub-Algorithm SubAlg1",sc);

  sc =  createSubAlgorithm( "SubAlg", "SubAlg2", m_subalg2);
  if( sc.isFailure() ) return Error("Error creating Sub-Algorithm SubAlg2",sc);

  return StatusCode::SUCCESS;
}

//------------------------------------------------------------------------------
StatusCode ParentAlg::execute() {
//------------------------------------------------------------------------------
  MsgStream         log( msgSvc(), name() );
  StatusCode sc;
  log << MSG::INFO << "executing...." << endreq;

  std::vector<Algorithm*>::const_iterator it  = subAlgorithms()->begin();
  std::vector<Algorithm*>::const_iterator end = subAlgorithms()->end();
  for ( ; it != end; it++) {
    sc = (*it)->execute();
    if( sc.isFailure() ) {
      log << "Error executing Sub-Algorithm" << (*it)->name() << endreq;
    }
  }
  return StatusCode::SUCCESS;
}


//------------------------------------------------------------------------------
StatusCode ParentAlg::finalize() {
//------------------------------------------------------------------------------
  MsgStream log(msgSvc(), name());
  log << MSG::INFO << "finalizing...." << endreq;
  return StatusCode::SUCCESS;
}