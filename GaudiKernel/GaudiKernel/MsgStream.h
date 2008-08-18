// $Id: MsgStream.h,v 1.42 2007/05/22 09:20:48 hmd Exp $
#ifndef GAUDIKERNEL_MSGSTREAM_H
#define GAUDIKERNEL_MSGSTREAM_H

// Include files
#include "GaudiKernel/IMessageSvc.h"
// Standard C++ classes
#include <cstdio>
#include <string>
#include <iomanip>
#include <vector>
#include <sstream>

/**
 * @class MsgStream MsgStream.h GaudiKernel/MsgStream.h
 *
 * Definition of the MsgStream class used to transmit messages.
 * This class is intended to ease the use of error logging to the message
 * service
 *
 * @author M.Frank
 * @author Sebastien Ponce
 */
class MsgStream     {
private:
  /** Error return code in case ios modification is requested for inactive
   *  streams
   */
  typedef std::ios_base::fmtflags FLAG_TYPE;
  typedef std::ios_base::iostate  STATE_TYPE;
protected:
  /// Pointer to message service if buffer has send
  IMessageSvc*    m_service;
  /// Use standard string for information buffering
  std::string     m_buffer;
  /// Use std::string for source information to be passed to the message service
  std::string     m_source;
  /// String MsgStream associated to buffer
  std::ostringstream m_stream;
  /// Flag set to true if formatting engine is active
  bool            m_active;
  /// Debug level of the message service
  MSG::Level      m_level;
  /// Current debug level
  MSG::Level      m_currLevel;
  /// use colors
  bool m_useColors;
public:
  /// Standard constructor: Connect to message service for output
  MsgStream(IMessageSvc* svc, int buffer_length=128);
  /// Standard constructor: Connect to message service for output
  MsgStream(IMessageSvc* svc, const std::string& source, int buffer_length=128);
  /// Copy constructor
  MsgStream(const MsgStream& msg)
    : m_service(msg.m_service),
      m_source(msg.m_source),
      m_active(msg.m_active),
      m_level(msg.m_level),
      m_useColors(msg.m_useColors)
  {
  }
  /// Standard destructor
  virtual ~MsgStream();
  /// Initialize report of new message: activate if print level is sufficient.
  MsgStream& report(int lvl)   {
    lvl = (lvl >= MSG::NUM_LEVELS) ?
      MSG::ALWAYS : (lvl<MSG::NIL) ? MSG::NIL : lvl;
    ((m_currLevel=MSG::Level(lvl)) >= level()) ? activate() : deactivate();
    return *this;
  }
  /// Output method
  virtual MsgStream& doOutput();
  /// Access string buffer
  const std::string& buffer() const {
    return m_buffer;
  }
  /// Access string MsgStream
  std::ostringstream& stream()   {
    return m_stream;
  }
  /// Update @c IMessageSvc pointer
  void setMsgSvc( IMessageSvc* svc ) {
    m_service = svc;
  }
  /// Update outputlevel
  void setLevel(int level)    {
    level = (level >= MSG::NUM_LEVELS) ?
      MSG::ALWAYS : (level<MSG::NIL) ? MSG::NIL : level;
    m_level = MSG::Level(level);
  }
  /// Retrieve output level
  MSG::Level level()   {
    return m_level;
  }
  /// Retrieve current stream output level
  MSG::Level currentLevel()   {
    return m_currLevel;
  }
  /// Activate MsgStream
  void activate()     {
    m_active = true;
  }
  /// Deactivate MsgStream
  void deactivate()     {
    m_active = false;
  }
  /// Accessor: is MsgStream active
  bool isActive()  const   {
    return m_active;
  }
  // oMsgStream flush emulation
  MsgStream& flush()    {
    if ( isActive() ) m_stream.flush();
    return *this;
  }
  // oMsgStream write emulation
  MsgStream& write(const char* buff,int len)  {
    if ( isActive() ) m_stream.write(buff, len);
    return *this;
  }
  /// Accept MsgStream modifiers
  MsgStream& operator<<(MsgStream& (*_f)(MsgStream&))    {
    if ( isActive() ) _f(*this);
    return *this;
  }
  /// Accept oMsgStream modifiers
  MsgStream& operator<<(std::ostream& (*_f)(std::ostream&))    {
    if ( isActive() ) _f(m_stream);
    return *this;
  }
  /// Accept ios modifiers
  MsgStream& operator<<(std::ios& (*_f)(std::ios&))    {
    if ( isActive() ) _f(m_stream);
    return *this;
  }
  /// Accept MsgStream activation using MsgStreamer operator
  MsgStream& operator<< (MSG::Level level)  {
    return report(level);
  }
  MsgStream& operator<<(longlong arg) {
    if(isActive()) {
#ifdef _WIN32
      int flg = m_stream.flags();
      char buf[128];
      (flg & std::ios::hex) ?
        ::sprintf(buf,"%I64x",arg) : ::sprintf(buf,"%I64d",arg);
      m_stream << buf;
#else
      m_stream << arg;
#endif
    }
    return *this;
  }

  /// Accept ios base class modifiers
  MsgStream& operator<<(std::ios_base& (*_f)(std::ios_base&))    {
    if ( isActive() ) _f(m_stream);
    return *this;
  }

  /// IOS emulation
  long flags() const {
    return isActive() ? m_stream.flags()    : 0;
  }
  long flags(FLAG_TYPE v) {
    return isActive() ? m_stream.flags(v)  :  0;
  }
  long setf(FLAG_TYPE v) {
    return isActive() ? m_stream.setf(v)  :  0;
  }
  int width() const {
    return isActive() ? m_stream.width()    : 0;
  }
  int width(int v) {
    return isActive() ? m_stream.width(v)    : 0;
  }
  char fill() const {
    return isActive() ? m_stream.fill()     : -1;
  }
  char fill(char v) {
    return isActive() ? m_stream.fill(v)     : -1;
  }
  int precision() const  {
    return isActive() ? m_stream.precision(): 0;
  }
  int precision(int v) {
    return isActive() ? m_stream.precision(v): 0;
  }
  int rdstate() const  {
    return isActive() ? m_stream.rdstate () : std::ios_base::failbit;
  }
  int good() const  {
    return isActive() ? m_stream.good ()    : 0;
  }
  int eof() const  {
    return isActive() ? m_stream.eof ()     : 0;
  }
  int bad() const  {
    return isActive() ? m_stream.bad()      : 0;
  }
  long setf(FLAG_TYPE _f, FLAG_TYPE _m) {
    return isActive() ? m_stream.setf(_f, _m)   : 0;
  }
  void unsetf(FLAG_TYPE _l)    {
    if ( isActive() ) m_stream.unsetf(_l);
  }
  void clear(STATE_TYPE _i = std::ios_base::failbit)  {
    if ( isActive() ) m_stream.clear(_i);
  }

  /// Set the text color
  void setColor(MSG::Color col);
  /// Set the foreground and background colors
  void setColor(MSG::Color fg, MSG::Color bg);

  /// Reset the colors to defaults
  void resetColor();

};

/// MsgStream Modifier: endreq. Calls the output method of the MsgStream
inline MsgStream& endreq(MsgStream& s)   {
  return s.doOutput();
}
/// MsgStream Modifier: endreq. Calls the output method of the MsgStream
inline MsgStream& endmsg(MsgStream& s)   {
  return s.doOutput();
}

/// MsgStream format utility "a la sprintf(...)"
std::string format(const char*, ... );

#ifdef _WIN32
template<class _E> inline
MsgStream& operator<<( MsgStream& s, const std::_Fillobj<_E>& obj)    {
#if _MSC_VER > 1300
  if ( s.isActive() ) s.stream().fill(obj._Fill);
#else
  if ( s.isActive() ) s.stream().fill(obj._Ch);
#endif
  return s;
}
template<class _Tm> inline
MsgStream& operator << (MsgStream& s, const std::_Smanip<_Tm>& manip) {
#if _MSC_VER > 1300
  if ( s.isActive() ) (*manip._Pfun)(s.stream(), manip._Manarg);
#else
  if ( s.isActive() ) (*manip._Pf)(s.stream(), manip._Manarg);
#endif
  return s;
}
#elif defined (__GNUC__)
inline MsgStream& operator << (MsgStream& s,
                               const std::_Setiosflags &manip) {
  if ( s.isActive() ) s.stream() << manip;
  return s;
}
inline MsgStream& operator << (MsgStream& s,
                               const std::_Resetiosflags &manip)      {
  if ( s.isActive() ) s.stream() << manip;
  return s;
}
inline MsgStream& operator << (MsgStream& s,
                               const std::_Setbase &manip)    {
  if ( s.isActive() ) s.stream() << manip;
  return s;
}
inline MsgStream& operator << (MsgStream& s,
                               const std::_Setprecision &manip)       {
  if ( s.isActive() ) s.stream() << manip;
  return s;
}
inline MsgStream& operator << (MsgStream& s,
                               const std::_Setw &manip)       {
  if ( s.isActive() ) s.stream() << manip;
  return s;
}

namespace MSG {
  inline
  MsgStream& dec(MsgStream& log) {
    log.setf(std::ios_base::dec, std::ios_base::basefield);
    return log;
  }
  inline
  MsgStream& hex(MsgStream& log) {
    log.setf(std::ios_base::hex, std::ios_base::basefield);
    return log;
  }
}

#else // GCC, version << 3
/// I/O Manipulator for setfill
template<class _Tm> inline
MsgStream& operator << (MsgStream& s, const std::smanip<_Tm>& manip)  {
  if ( s.isActive() ) s.stream() << manip;
  return s;
}
#endif    // WIN32 or (__GNUC__)

/// General templated stream operator
template <typename T>
MsgStream& operator<< (MsgStream& lhs, const T& arg)  {
  if(lhs.isActive()) lhs.stream() << arg;
  return lhs;
}
/// Specialization stream operator for std::vector<T>
template <typename T>
MsgStream& operator<< (MsgStream& lhs, const std::vector<T>& v ) {
  if(lhs.isActive())
    for ( typename std::vector<T>::const_iterator i = v.begin();
          i != v.end(); ++i ) { lhs.stream() << *i << " "; }
  return lhs;
}
/// Specialization stream operator for std::vector<T,A>
template <typename T, typename A>
MsgStream& operator<< (MsgStream& lhs, const std::vector<T,A>& v ) {
  if(lhs.isActive())
    for ( typename std::vector<T,A>::const_iterator i = v.begin();
          i != v.end(); ++i ) { lhs.stream() << *i << " "; }
  return lhs;
}

#ifdef __GNUC__
/// compiler is stupid. Must specialize
template<typename T>
MsgStream& operator << (MsgStream& lhs, const std::_Setfill<T> &manip) {
  if ( lhs.isActive() ) lhs.stream() << manip;
  return lhs;
}
#endif

#endif    // GAUDIKERNEL_MSGSTREAM_H
