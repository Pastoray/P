#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "diagnostic.hpp"
#include "utils.hpp"
#include <optional>
#include <string>
#include <variant>
#include <vector>

#define OP(name, str)
#define SYMBOL_LIST \
  OP(LBRA, "[") \
  OP(RBRA, "]") \
  OP(LPAR, "(") \
  OP(RPAR, ")") \
  OP(LCUR, "{") \
  OP(RCUR, "}") \
  OP(COM, ",") \
  OP(AEQ, "=") \
  OP(EQ, "==") \
  OP(NEQ, "!=") \
  OP(AST, "*") \
  OP(SLSH, "/") \
  OP(ADD, "+") \
  OP(SUB, "-") \
  OP(INC, "++") \
  OP(DEC, "--") \
  OP(ARROW, "->") \
  OP(GT, ">") \
  OP(LT, "<") \
  OP(GTE, ">=") \
  OP(LTE, "<=") \
  OP(AMP, "&") \
  OP(PIPE, "|") \
  OP(NOT, "!") \
  OP(DOT, ".") \
  OP(COL, ":") \
  OP(NSR, "::") \
  OP(QM, "\"") \
  OP(SEMICOL, ";")

#define LITERAL_LIST \
  OP(INT, "int") \
  OP(BOOL, "bool") \
  OP(DOUBLE, "double") \
  OP(STRING, "string") \
  OP(CHAR, "char") \
  OP(IDENT, "ident")

#define KEYWORD_LIST \
  OP(IF, "if") \
  OP(ELSE, "else") \
  OP(FOR, "for") \
  OP(CONTINUE, "continue") \
  OP(BREAK, "break") \
  OP(WHILE, "while") \
  OP(FN, "fn") \
  OP(RET, "return") \
  OP(STRUCT, "struct") \
  OP(UNION, "union") \
  OP(ENUM, "enum") \
  OP(IMPORT, "import") \
  OP(NAMEPSACE, "namespace")

namespace TokenTypes
{
  enum class Symbol
  {
    #undef OP
    #define OP(name, str) name,
    SYMBOL_LIST
    #undef OP
  };

  enum class Literal
  {
    #undef OP
    #define OP(name, str) name,
    LITERAL_LIST
    #undef OP
  };

  enum class Keyword
  {
    #undef OP
    #define OP(name, str) name,
    KEYWORD_LIST
    #undef OP
  };
}

using TokenType = std::variant<TokenTypes::Symbol, TokenTypes::Keyword, TokenTypes::Literal>;

struct Token
{
  explicit Token(const TokenType t, SrcLoc loc) : type(t), loc(std::move(loc)) {}
  Token(const TokenType t, const std::string& s, SrcLoc loc) : type(t), value(s), loc(std::move(loc)) {}

  template <typename T>
  bool operator==(const T val) const
  {
    static_assert(
      std::is_same_v<T, TokenTypes::Symbol> ||
      std::is_same_v<T, TokenTypes::Keyword> ||
      std::is_same_v<T, TokenTypes::Literal>
    );
    return (std::holds_alternative<T>(type) && std::get<T>(type) == val);
  }

  static std::string to_string(const Token& tok)
  {
    return std::visit(
      Utils::overloaded
      {
        [&](const TokenTypes::Symbol& arg)
        {
          switch (arg)
          {
            #define OP(name, str) case TokenTypes::Symbol::name: return str;
            SYMBOL_LIST
            #undef OP
            default: return "Unknown symbol";
          }
        },
        [&](const TokenTypes::Literal& arg)
        {
          switch (arg)
          {
            #define OP(name, str) case TokenTypes::Literal::name: return str;
            LITERAL_LIST
            #undef OP
            default: return "Unknown literal";
          }
        },
        [&](const TokenTypes::Keyword& arg)
        {
          switch (arg)
          {
            #define OP(name, str) case TokenTypes::Keyword::name: return str;
            KEYWORD_LIST
            #undef OP
            default: return "Unknown keyword";
          }
        }
      }, tok.type
    );
  }
  std::ostream& operator<<(std::ostream& os) const
  {
    os << to_string(*this);
    return os;
  }

  template <typename T>
  bool operator!=(const T val) const
  {
    return !(*this == val);
  }

  TokenType type;
  std::optional<std::string> value;
  SrcLoc loc;

  template <typename T>
  bool operator==(const T& type_enum)
  {
    if (auto* op = std::get_if<T>(&type))
      return *op == type_enum;
    return false;
  }

  friend std::ostream& operator<<(std::ostream& os, const Token& token)
  {
    os << to_string(token);
    return os;
  }
};

template <>
struct fmt::formatter<Token> : fmt::formatter<string_view>
{
  auto format(const Token& tok, format_context& ctx) const -> format_context::iterator
  {
    std::string text = Token::to_string(tok);
    return fmt::formatter<string_view>::format(text, ctx);
  }
};

template <typename T>
bool operator==(const T val, const Token& t)
{
  static_assert(
    std::is_same_v<T, TokenTypes::Symbol> ||
    std::is_same_v<T, TokenTypes::Keyword> ||
    std::is_same_v<T, TokenTypes::Literal>
  );
  return t == val;
}

template <typename T>
bool operator!=(const T val, const Token& t)
{
  return t != val;
}

class Tokenizer
{
public:
  explicit Tokenizer(std::string& src);
  explicit Tokenizer(std::string&& src);
  std::vector<Token> tokenize();
  static std::string tokentype_to_string(TokenType type);

private:
  [[nodiscard]] std::optional<char> peek(int offset = 0) const;
  char consume(unsigned int amount = 1);
  void sanitize_src();

private:
  std::string m_src;
  size_t m_index;
};

#endif // TOKENIZER_H
