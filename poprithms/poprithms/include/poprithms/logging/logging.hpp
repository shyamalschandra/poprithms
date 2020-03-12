#ifndef POPRITHMS_LOGGING_LOGGING_HPP
#define POPRITHMS_LOGGING_LOGGING_HPP

#include <memory>
#include <string>

namespace poprithms {
namespace logging {

class LoggerImpl;

enum class Level { Trace = 0, Debug, Info, Off };
std::ostream &operator<<(std::ostream &, Level);

// Set the logging level for all Loggers. Example:
//
//                                A     B     C     D
// Logger A("a");                Off   --    --    --
// Logger B("b");                Off   Off   --    --
// setGlobalLevel(Level::Info);  Info  Info  --    --
// Logger C("c");                Info  Info  Info  --
// setGlobalLevel(Level::Debug); Debug Debug Debug --
// B.setLevel(Level::Off);       Debug Off   Debug --
// setGlobalLevel(Level::Info);  Info  Info  Info  --
// Logger D("d");                Info  Info  Info  Info
// A.setLevel(Level::Off);       Off   Info  Info  Info
// Logger E("a");                Error: cannot have 2 Loggers with same name
//
//

void setGlobalLevel(Level);

class Logger {
public:
  Logger(const std::string &id);
  ~Logger();
  void info(const std::string &) const;
  void debug(const std::string &) const;
  void trace(const std::string &) const;

  void setLevel(Level);
  void setLevelInfo() { setLevel(Level::Info); }
  void setLevelDebug() { setLevel(Level::Debug); }
  void setLevelTrace() { setLevel(Level::Trace); }
  void setLevelOff() { setLevel(Level::Off); }

  Level getLevel() const;

  //
  // In the following table, at "x" logging will be produced
  //
  //                                 getLevel()
  //                        -----------------------
  //                        Trace  Debug  Info  Off
  //
  //            Trace         x      .     .     .
  // atLevel    Debug         x      x     .     .
  //            Info          x      x     x     .
  //
  bool shouldLog(Level atLevel) const;
  bool shouldLogInfo() const { return shouldLog(Level::Info); }
  bool shouldLogDebug() const { return shouldLog(Level::Debug); }
  bool shouldLogTrace() const { return shouldLog(Level::Trace); }

private:
  LoggerImpl *impl;
};

} // namespace logging
} // namespace poprithms

#endif
