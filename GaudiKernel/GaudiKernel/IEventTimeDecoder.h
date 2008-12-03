// $Id: IEventTimeDecoder.h,v 1.1 2008/07/17 13:29:55 marcocle Exp $
#ifndef GAUDIKERNEL_IEVENTTIMEDECODER_H 
#define GAUDIKERNEL_IEVENTTIMEDECODER_H 1

// Include files
// from STL
#include <string>

// from Gaudi
#include "GaudiKernel/IAlgTool.h"
#include "GaudiKernel/Time.h"

static const InterfaceID IID_IEventTimeDecoder ( "IEventTimeDecoder", 1, 0 );

/** @class IEventTimeDecoder IEventTimeDecoder.h GaudiKernel/IEventTimeDecoder.h
 *  
 *  Interface that a Tool that decodes the event time has to implement.
 *
 *  @author Marco Clemencic
 *  @date   2006-09-21
 */
class IEventTimeDecoder : virtual public IAlgTool {
public: 

  // Return the interface ID
  static const InterfaceID& interfaceID() { return IID_IEventTimeDecoder; }

  /// Return the time of current event.
  virtual Gaudi::Time getTime() const = 0;

};
#endif // GAUDIKERNEL_IEVENTTIMEDECODER_H
