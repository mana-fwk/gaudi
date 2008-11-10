// $Id: EvtDataSvc.h,v 1.4 2006/11/30 14:57:03 mato Exp $
#ifndef DATASVC_EVTDATASVC_H
#define DATASVC_EVTDATASVC_H

#include "GaudiKernel/DataSvc.h"
class IConversionSvc;


/** @class EvtDataSvc EvtDataSvc.h
 * 
 *   A EvtDataSvc is the base class for event services
 *
 *  @author M.Frank
 */
class EvtDataSvc  : public DataSvc   {
  friend class SvcFactory<EvtDataSvc>;

public:
  virtual StatusCode initialize();
  virtual StatusCode reinitialize();
  virtual StatusCode finalize();

  /// Standard Constructor
  EvtDataSvc(const std::string& name, ISvcLocator* svc);

  /// Standard Destructor
  virtual ~EvtDataSvc();
private:
  IConversionSvc* m_cnvSvc;
};
#endif // DATASVC_EVTDATASVC_H
