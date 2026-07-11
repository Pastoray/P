#include "tokenizer.hpp"
#include "utils.hpp"
#include <cassert>

Tokenizer::Tokenizer(std::string& src) : m_src(std::move(src)), m_index(0) {}
Tokenizer::Tokenizer(std::string&& src) : m_src(std::move(src)), m_index(0) {}

void Tokenizer::sanitize_src()
{
  std::string new_src;
  new_src.reserve(m_src.size() * 2);
  for (auto& c : m_src)
  {
    if (c == '\t') new_src.append("  ");
    else new_src.push_back(c);
  }
  m_src = std::move(new_src);
}

std::vector<Token> Tokenizer::tokenize()
{
  sanitize_src();
  size_t row = 1, col = 1;
  auto consume = [&](const unsigned int amount = 1) -> char
  {
    col += amount;
    return this->consume(amount);
  };

  std::vector<Token> tokens;
  std::string buffer;
  while (peek().has_value())
  {
    std::string f;
    SrcLoc loc(f, row, col, m_src);
    if (std::isdigit(peek().value()))
    {
      while (peek().has_value() && std::isdigit(peek().value()))
      {
        buffer += m_src[m_index];
        consume();
      }
      if (peek() && peek() == '.')
      {
        buffer += m_src[m_index];
        consume();
        while (peek().has_value() && std::isdigit(peek().value()))
        {
          buffer += m_src[m_index];
          consume();
        }

        tokens.emplace_back(TokenTypes::Literal::DOUBLE, buffer, loc);
      }
      else tokens.emplace_back(TokenTypes::Literal::INT, buffer, loc);
      buffer.clear();
    }
    else if (std::isalpha(peek().value()) || peek() == '_')
    {
      buffer += peek().value();
      consume();
      while (peek().has_value() && (std::isalnum(peek().value()) || peek().value() == '_'))
      {
        buffer += m_src[m_index];
        consume();
      }
      #define OP(name, str) if (buffer == str) { tokens.emplace_back(TokenTypes::Keyword::name, loc); } else
      KEYWORD_LIST
      #undef OP
      if (buffer == "true" || buffer == "false") { tokens.emplace_back(TokenTypes::Literal::BOOL, buffer, loc); }
      else { tokens.emplace_back(TokenTypes::Literal::IDENT, buffer, loc); }
      buffer.clear();
    }
    else if (peek().value() == '=')
    {
      consume();
      if (peek() && peek().value() == '=')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::EQ, loc);
      }
      else
        tokens.emplace_back(TokenTypes::Symbol::AEQ, loc);
    }
    else if (peek().value() == '[')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::LBRA, loc);
    }
    else if (peek().value() == ']')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::RBRA, loc);
    }
    else if (peek().value() == '(')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::LPAR, loc);
    }
    else if (peek().value() == ')')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::RPAR, loc);
    }
    else if (peek().value() == '+')
    {
      consume();
      if (peek() && peek().value() == '+')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::INC, loc);
      }
      else
        tokens.emplace_back(TokenTypes::Symbol::ADD, loc);
    }
    else if (peek().value() == '-')
    {
      consume();
      if (peek() && peek().value() == '-')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::DEC, loc);
      }
      else if (peek() && peek().value() == '>')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::ARROW, loc);
      }
      else
        tokens.emplace_back(TokenTypes::Symbol::SUB, loc);
    }
    else if (peek().value() == '*')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::AST, loc);
    }
    else if (peek().value() == '/')
    {
      consume();
      if (peek().has_value() && peek().value() == '/')
      {
        while (peek().has_value() && peek().value() != '\n') consume();
        row++;
        col = 1;
      }
      else if (peek().has_value() && peek().value() == '*')
      {
        consume();
        while (peek().has_value() && peek().value() != '*' && peek(1).has_value() && peek(1).value() != '/') consume();
        consume();
        if (!peek().has_value())
        {
          Utils::panic("Tokenization failed due to unclosed comment");
        }
        consume();
      }
      else
      {
        tokens.emplace_back(TokenTypes::Symbol::SLSH, loc);
      }
    }
    else if (peek().value() == '>')
    {
      consume();
      if (peek() && peek().value() == '=')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::GTE, loc);
      }
      else
        tokens.emplace_back(TokenTypes::Symbol::GT, loc);
    }
    else if (peek().value() == '<')
    {
      consume();
      if (peek() && peek().value() == '=')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::LTE, loc);
      }
      else
        tokens.emplace_back(TokenTypes::Symbol::LT, loc);
    }
    else if (peek().value() == '&')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::AMP, loc);
    }
    else if (peek().value() == '|')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::PIPE, loc);
    }
    else if (peek().value() == '!')
    {
      consume();
      if (peek() && peek().value() == '=')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::NEQ, loc);
      }
      else
        tokens.emplace_back(TokenTypes::Symbol::NOT, loc);
    }
    else if (peek().value() == ',')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::COM, loc);
    }
    else if (peek().value() == ';')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::SEMICOL, loc);
    }
    else if (peek().value() == ':')
    {
      consume();
      if (peek() && peek() == ':')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::NSR, loc);
      }
      else tokens.emplace_back(TokenTypes::Symbol::COL, loc);
    }
    else if (peek().value() == '}')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::RCUR, loc);
    }
    else if (peek().value() == '{')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::LCUR, loc);
    }
    else if (peek().value() == '.')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::DOT, loc);
    }
    else if (peek().value() == '\"')
    {
      consume();
      for (; peek() && peek() != '\"'; buffer += consume());
      assert(peek() && peek() == '\"');
      consume();

      tokens.emplace_back(TokenTypes::Literal::STRING, buffer, loc);
      buffer.clear();
    }
    else if (peek().value() == '\'')
    {
      consume();
      for (; peek() && peek() != '\''; buffer += consume());
      assert(peek() && peek() == '\'');
      consume();

      tokens.emplace_back(TokenTypes::Literal::CHAR, buffer, loc);
      buffer.clear();
    }
    else if (peek().value() == '\n')
    {
      row++;
      col = 1;
      consume();
    }
    else if (peek().value() == ' ' || peek().value() == '\t')
      consume();
  }
  return tokens;
}

[[nodiscard]]
std::optional<char> Tokenizer::peek(const int offset) const
{
  if (m_index + offset < m_src.size())
    return m_src[m_index + offset];
  return {};
}

char Tokenizer::consume(const unsigned int amount)
{
  if (amount == 0) [[unlikely]]
    Utils::panic("[INDEX: ", std::to_string(m_index), "] ",
                 "Consume called with a value of 0");

  m_index += amount;
  return peek(-1).value(); // Return last consumed token
}

