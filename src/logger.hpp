#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <ostream>

class Logger
{
public:
  Logger() = delete;
  template <typename... Args>
  static void log(Args&&... args) { ((stream << std::forward<Args>(args)), ...); }

  template <typename... Args>
  static void debug(Args&&... args)
  {
    stream << "[DEUBG] ";
    log(std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void info(Args&&... args)
  {
    stream << "[INFO] ";
    log(std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void warn(Args&&... args)
  {
    stream << "[WARN] ";
    log(std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void error(Args&&... args)
  {
    stream << "[ERROR] ";
    log(std::forward<Args>(args)...);
  }

private:
  static inline std::ostream& stream = std::cout;
};



#endif //LOGGER_H
