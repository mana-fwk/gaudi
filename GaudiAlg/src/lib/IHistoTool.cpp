// $Id: IHistoTool.cpp,v 1.2 2005/01/18 15:51:53 mato Exp $
// ============================================================================
// Include files
// ============================================================================
// GaudiKernel
// ============================================================================
#include "GaudiKernel/IInterface.h"
// ============================================================================
// local
// ============================================================================
#include "GaudiAlg/IHistoTool.h"
// ============================================================================

// ============================================================================
/** @file IHistoTool.cpp
 *
 *  Implementation file for class : IHistoTool
 *  @date 2004-06-28 
 *  @author Vanya BELYAEV Ivan.Belyaev@itep.ru
 */
// ============================================================================

namespace 
{
  const InterfaceID IID_IHistoTool ( "IHistoTool" , 1 , 0 ) ;
};

// ============================================================================
/// Return the unique interface ID
// ============================================================================
const InterfaceID& IHistoTool::interfaceID() { return IID_IHistoTool ; }
// ============================================================================

// ============================================================================
/// detsructor 
// ============================================================================
IHistoTool::~IHistoTool() {}
// ============================================================================
