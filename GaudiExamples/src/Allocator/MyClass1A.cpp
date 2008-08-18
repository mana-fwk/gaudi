// $Id: MyClass1A.cpp,v 1.1 2006/02/14 15:07:07 hmd Exp $
// ============================================================================
// CVS tag $NAme:$, version $Revision: 1.1 $
// ============================================================================
// CVS tag $Name:  $ 
// ============================================================================
// Include files
// ============================================================================
// GaudiKernel
// ============================================================================
#include "GaudiKernel/Allocator.h"
// ============================================================================
// Local
// ============================================================================
#include "MyClass1A.h"
// ============================================================================

/** @file 
 *  Implementation file for class MyClass1
 *  @date 2006-02-14 
 *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
 */

MyClass1A::MyClass1A(){}
MyClass1A::~MyClass1A(){}


// ============================================================================
// Anonymous namespace to hide the allocator 
// ============================================================================
namespace 
{
  GaudiUtils::Allocator<MyClass1A> s_Allocator ;
};
// ============================================================================

// ============================================================================
/// overloaded 'new' operator 
// ============================================================================
void* MyClass1A::operator new(size_t)
{
  void *hit  ;
  hit = (void *) s_Allocator.MallocSingle () ;
  return hit ;
};
// ============================================================================

// ============================================================================
/// overloaded 'delete' operator 
// ============================================================================
void MyClass1A::operator delete( void *hit )
{ s_Allocator.FreeSingle( (MyClass1A*) hit ); };
// ============================================================================

// ============================================================================
// The END 
// ============================================================================