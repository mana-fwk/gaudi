// $Id: FuncMinimumPAlg.cpp,v 1.4 2006/01/10 19:58:27 hmd Exp $

// Include files
// from Gaudi
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/MsgStream.h" 
#include "GaudiGSL/IFuncMinimum.h"
#include "GaudiMath/Adapters.h"
#include "GaudiMath/GaudiMath.h"
// from CLHEP
#include "CLHEP/Matrix/SymMatrix.h"
// local
#include "FuncMinimumPAlg.h"

//-----------------------------------------------------------------------------
/** @file Implementation file for class : FuncMinimumPAlg
 *  @see FuncMinimumPAlg.h
 *  @author Kirill Miklyaev kirillm@iris1.itep.ru
 *  @date 2002-09-14
 */
//-----------------------------------------------------------------------------

// Declaration of the Algorithm Factory
DECLARE_ALGORITHM_FACTORY(FuncMinimumPAlg)


//=============================================================================
// Standard constructor, initializes variables
//=============================================================================
FuncMinimumPAlg::FuncMinimumPAlg( const std::string& name,
                                ISvcLocator* pSvcLocator)
  : Algorithm ( name , pSvcLocator ) {

}

//=============================================================================
// Destructor
//=============================================================================
FuncMinimumPAlg::~FuncMinimumPAlg() {}; 

//=============================================================================
// Our function
double function ( const std::vector<double>& x  )
{
  return 10 * x[0] * x[0] + 20 * x[1] * x[1] + 40;
};

//=============================================================================
// Initialisation. Check parameters
//=============================================================================
StatusCode FuncMinimumPAlg::initialize() {
  
  MsgStream log(msgSvc(), name());
  log << MSG::INFO << "==> Initialise" << endreq;

  StatusCode sc;
  sc = toolSvc()->retrieveTool("FuncMinimum", m_publicTool );
  if( sc.isFailure() ) 
    {
      log << MSG::ERROR<< "Error retrieving the public tool" << endreq;  
    }
  sc = toolSvc()->retrieveTool("FuncMinimum", m_privateTool, this );
  if( sc.isFailure() ) 
    {
      log << MSG::ERROR<< "Error retrieving the private tool" << endreq;  
    }
  log << MSG::INFO << "....initialization done" << endreq;
  
  return StatusCode::SUCCESS;
};

//=============================================================================
// Main execution
//=============================================================================
StatusCode FuncMinimumPAlg::execute() {

  MsgStream  log( msgSvc(), name() );
  log << MSG::INFO << "==> Execute" << endreq;
  
  // the object of the class AdapterPFunction
  // @see Adapter.h
  const GaudiMath::Function& func = GaudiMath::adapter( 2 , &function ) ;
  
 //=============================================================================
  // Input number and value of the arguments of the function "GenFunc"
  IFuncMinimum::Arg arg (func.dimensionality ());

  arg[0] = 2;
  arg[1] = 4;

  // Matrix of error
  IFuncMinimum::Covariance matrix_error (arg.dimension(), 0);
  
  // Call of the method
  m_publicTool->minimum( func ,
                         arg  );
  log << endreq;
  log << "START OF THR METHOD" << endreq;
  log << "MINIMUM FOUND AT: " << endreq;
  
  for (unsigned int i = 0; i < arg.dimension(); i++)
    {
      
      log << "Value of argument " << i <<" is " << arg[i] << endreq;
    }
  
  log << endreq;
//=============================================================================
  // With Covariance matrix (matrix of error)  
  arg[0] = 2;
  arg[1] = 4;
  
  // Call of the method(with covariance matrix (matrix of error))
  m_publicTool->minimum( func        ,
                         arg         ,
                         matrix_error);
  log << endreq;
  log << "THE METHOD WITH MATRIX OF ERROR" << endreq;
  log << "MINIMUM FOUND AT: " << endreq;

  for (unsigned int i = 0; i < arg.dimension(); i++)
    {

      log << "Value of argument " << i <<" is " << arg[i] << endreq;
    }

  log << endreq;
  log << "MATRIX OF ERROR";

  for (unsigned int i = 0; i < arg.dimension(); i++)
    {
      log << endreq;

      for (unsigned int j = 0; j < arg.dimension(); j++)
        {
          log << matrix_error (i+1, j+1) << " ";
        }
    }
  log << endreq;

  return StatusCode::SUCCESS;
};

//=============================================================================
//  Finalize
//=============================================================================
StatusCode FuncMinimumPAlg::finalize() {

  MsgStream log(msgSvc(), name());
  log << MSG::INFO << "==> Finalize" << endreq;

  toolSvc()->releaseTool( m_publicTool  );
  toolSvc()->releaseTool( m_privateTool );

  return StatusCode::SUCCESS;
}

//=============================================================================