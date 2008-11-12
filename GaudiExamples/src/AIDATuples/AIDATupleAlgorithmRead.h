// $Id: AIDATupleAlgorithmRead.h,v 1.2 2005/12/13 13:29:24 hmd Exp $
#ifndef AIDATUPLES_AIDATupleAlgorithmRead_H
#define AIDATUPLES_AIDATupleAlgorithmRead_H 1

// Include files
#include "GaudiKernel/Algorithm.h"
#include "AIDA/ITuple.h"

using namespace AIDA;

// Forward declarations
class AIDATupleSvc;

class AIDATupleAlgorithmRead : public Algorithm {

public:
  /// Constructor of this form must be provided
  AIDATupleAlgorithmRead(const std::string& name, ISvcLocator* pSvcLocator);

  /// Three mandatory member functions of any algorithm
  StatusCode initialize();
  StatusCode execute();
  StatusCode finalize();

private:

  AIDA::ITuple*   tuple;
   
};

#endif    // AIDATUPLES_AIDATupleAlgorithmRead_H
