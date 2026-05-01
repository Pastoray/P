#include "tokenizer.hpp"
#include "utils.hpp"

Tokenizer::Tokenizer(std::string& src) : m_src(std::move(src)), m_index(0) {}
Tokenizer::Tokenizer(std::string&& src) : m_src(std::move(src)), m_index(0) {}

std::vector<Token> Tokenizer::tokenize()
{
  std::vector<Token> tokens;
  std::string buffer;
  while (peek().has_value())
  {
    if (std::isdigit(peek().value()))
    {
      while (peek().has_value() && std::isdigit(peek().value()))
      {
        // Logger::debug("digit index: ", m_index);
        buffer += m_src[m_index];
        consume();
      }
      {
        tokens.emplace_back(TokenTypes::Literal::INT, buffer);
      }

      buffer.clear();
    }
    else if (std::isalpha(peek().value()))
    {
      buffer += peek().value();
      consume();
      while (peek().has_value() && (std::isalnum(peek().value()) || peek().value() == '_'))
      {
        buffer += m_src[m_index];
        consume();
      }
      if (buffer == "return")
      {
        tokens.emplace_back(TokenTypes::Keyword::RET);
      }
      else if (buffer == "if")
      {
        tokens.emplace_back(TokenTypes::Keyword::IF);
      }
      else if (buffer == "else")
      {
        tokens.emplace_back(TokenTypes::Keyword::ELSE);
      }
      else if (buffer == "fn")
      {
        tokens.emplace_back(TokenTypes::Keyword::FN);
      }
      else if (buffer == "for")
      {
        tokens.emplace_back(TokenTypes::Keyword::FOR);
      }
      else if (buffer == "continue")
      {
        tokens.emplace_back(TokenTypes::Keyword::CONTINUE);
      }
      else if (buffer == "break")
      {
        tokens.emplace_back(TokenTypes::Keyword::BREAK);
      }
      else if (buffer == "while")
      {
        tokens.emplace_back(TokenTypes::Keyword::WHILE);
      }
      else if (buffer == "import")
      {
        tokens.emplace_back(TokenTypes::Keyword::IMPORT);
      }
      else
      {
        tokens.emplace_back(TokenTypes::Literal::IDENT, buffer);
      }
      buffer.clear();
    }
    else if (peek().value() == '=')
    {
      consume();
      if (peek() && peek().value() == '=')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::EQ);
      }
      else
        tokens.emplace_back(TokenTypes::Symbol::AEQ);
    }
    else if (peek().value() == '[')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::LBRA);
    }
    else if (peek().value() == ']')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::RBRA);
    }
    else if (peek().value() == '(')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::LPAR);
    }
    else if (peek().value() == ')')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::RPAR);
    }
    else if (peek().value() == '+')
    {
      consume();
      if (peek() && peek().value() == '+')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::INC);
      }
      else
        tokens.emplace_back(TokenTypes::Symbol::ADD);
    }
    else if (peek().value() == '-')
    {
      consume();
      if (peek() && peek().value() == '-')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::DEC);
      }
      else
        tokens.emplace_back(TokenTypes::Symbol::SUB);
    }
    else if (peek().value() == '*')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::AST);
    }
    else if (peek().value() == '/')
    {
      consume();
      if (peek().has_value() && peek().value() == '/')
      {
        while (peek().has_value() && peek().value() != '\n')
          consume();
      }
      else if (peek().has_value() && peek().value() == '*')
      {
        consume();
        while (peek().has_value() && peek().value() != '*' && peek(1).has_value() && peek(1).value() != '/')
            consume();

        consume();
        if (!peek().has_value())
        {
          Utils::panic("Tokenization failed due to unclosed comment");
        }
        consume();
      }
      else
      {
        tokens.emplace_back(TokenTypes::Symbol::SLSH);
      }
    }
    else if (peek().value() == '>')
    {
      consume();
      if (peek() && peek().value() == '=')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::GTE);
      }
      else
        tokens.emplace_back(TokenTypes::Symbol::GT);
    }
    else if (peek().value() == '<')
    {
      consume();
      if (peek() && peek().value() == '=')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::LTE);
      }
      else
        tokens.emplace_back(TokenTypes::Symbol::LT);
    }
    else if (peek().value() == '&')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::AMP);
    }
    else if (peek().value() == '|')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::PIPE);
    }
    else if (peek().value() == '!')
    {
      consume();
      if (peek() && peek().value() == '=')
      {
        consume();
        tokens.emplace_back(TokenTypes::Symbol::NEQ);
      }
      else
        tokens.emplace_back(TokenTypes::Symbol::NOT);
    }
    else if (peek().value() == ',')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::COM);
    }
    else if (peek().value() == ';')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::SEMICOL);
    }
    else if (peek().value() == ':')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::COL);
    }
    else if (peek().value() == '}')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::RCUR);
    }
    else if (peek().value() == '{')
    {
      consume();
      tokens.emplace_back(TokenTypes::Symbol::LCUR);
    }
    else if (peek().value() == ' ' || peek().value() == '\n' ||
             peek().value() == '\t')
    {
      consume();
    }
  }
  return tokens;
}

[[nodiscard]] std::optional<char> Tokenizer::peek(const int offset) const
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

