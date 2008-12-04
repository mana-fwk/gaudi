#ifndef GAUDIPYTHON_CALLBACKSTREAMBUF_H
#define GAUDIPYTHON_CALLBACKSTREAMBUF_H
// ============================================================================
#include <sstream>    //included for stringbuf
// ============================================================================
#include "Python.h"   //included for the python API
// ============================================================================
#include "GaudiPython/GaudiPython.h"
// ============================================================================

namespace GaudiPython
{
  int call_python_method(PyObject* self, const char* method, char* buf);

  class CallbackStreamBuf : public  std::stringbuf
  {
    private:
      PyObject* m_self;
      char* m_callbackBuff;

    public:
      CallbackStreamBuf(PyObject* self);
      ~CallbackStreamBuf();
      virtual int sync() ;
  }; //CallbackStreamBuf

}; //namespace GaudiPython

#endif