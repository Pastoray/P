#ifndef UTILS_H
#define UTILS_H

#include "logger.hpp"

namespace Utils
{
  template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

  template<typename... Args>
  [[noreturn]] void panic(Args... args)
  {
    Logger::error(std::forward<Args>(args)..., '\n');
    std::exit(1);
  }
}

#endif // UTILS_H
