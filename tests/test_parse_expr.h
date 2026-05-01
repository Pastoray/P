#ifndef TEST_PARSE_EXPR_H
#define TEST_PARSE_EXPR_H

#include "../src/tokenizer.hpp"
#include "../src/parser.hpp"
#include <cassert>
#include <iostream>

inline void test_parse_expr_simple()
{
  std::string input = "42";

  Tokenizer tokenizer(input);
  auto tokens = tokenizer.tokenize();

  Parser parser(tokens);
  auto expr = parser.parse_expr();

  assert(expr.has_value());
  std::cout << "test_parse_expr_simple passed!\n";
}

inline void test_parse_expr_binop()
{
  std::string input = "1 + 2 * 3";

  Tokenizer tokenizer(input);
  auto tokens = tokenizer.tokenize();

  Parser parser(tokens);
  auto expr = parser.parse_expr();

  assert(expr.has_value());
  std::cout << "test_parse_expr_binop passed!\n";

}

inline void test_parse_expr()
{
  test_parse_expr_simple();
  test_parse_expr_binop();
}

#endif // TEST_PARSE_EXPR_H
