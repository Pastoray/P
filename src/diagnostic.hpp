#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include <iostream>
#include <string>
#include <fmt/core.h>
#include <fmt/format.h>

struct SrcLoc
{
  explicit SrcLoc(std::string file, size_t r, size_t c, std::string_view l)
    : file(std::move(file)), row(r), col(c), line(l) {}
  std::string file;
  size_t row;
  size_t col;
  std::string_view line;

  friend std::ostream& operator<<(std::ostream& os, const SrcLoc& loc)
  {
    os << loc.file << ":" << loc.row << ":" << loc.col;
    return os;
  }
};

class Diagnostic
{
  inline static size_t error_cnt = 0, warn_cnt = 0, note_cnt = 0;
  enum class Kind
  {
    ERROR,
    WARN,
    NOTE
  };

  public:
  template <typename... Args>
  static void error(const SrcLoc& loc, std::string_view str, Args&&... args)
  {
    error_cnt++;
    std::string msg = fmt::format(str, std::forward<Args>(args)...);
    report(Diagnostic::Kind::ERROR, loc, msg);
  }

  template <typename... Args>
  static void warn(const SrcLoc& loc, std::string_view str, Args&&... args)
  {
    warn_cnt++;
    std::string msg = fmt::format(str, std::forward<Args>(args)...);
    report(Diagnostic::Kind::WARN, loc, msg);
  }

  template <typename... Args>
  static void note(const SrcLoc& loc, std::string_view str, Args&&... args)
  {
    note_cnt++;
    std::string msg = fmt::format(str, std::forward<Args>(args)...);
    report(Diagnostic::Kind::NOTE, loc, msg);
  }

  private:
    inline static void report(Kind kind, const SrcLoc& loc, const std::string& msg)
    {
      std::string_view color;
      std::string_view label;
      
      switch (kind)
      {
        case Diagnostic::Kind::ERROR: color = "\033[1;31m"; label = "error"; break;
        case Diagnostic::Kind::WARN: color = "\033[1;35m"; label = "warning"; break;
        case Diagnostic::Kind::NOTE: color = "\033[1;36m"; label = "note"; break;
      }
      
      std::string_view reset = "\033[0m";
      std::string_view bold  = "\033[1m";

      std::cerr << bold << loc.file << ":" << loc.row << ":" << loc.col << ": "
        << color << label << ": " << reset << bold << msg << reset << '\n';

      if (!loc.line.empty())
      {
        std::cerr << " " << loc.row << " | " << loc.line << "\n";
        size_t pad = (loc.col > 0) ? (loc.col - 1) : 0;
        std::cerr << "   | " << std::string(pad, ' ') << color << "^" << reset << '\n';
      }
      std::cerr << std::endl;
    }
};

#endif // DIAGNOSTIC_H
