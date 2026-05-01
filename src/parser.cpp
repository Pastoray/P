#include "parser.hpp"
#include "tokenizer.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <memory>
#include <optional>
#include <variant>


uint32_t Node::Ident::nid = 0;
Parser::Parser(std::vector<Token>& tokens)
    : m_tokens(std::move(tokens)), m_index(0)
{
}

std::vector<Node::Node> Parser::parse_prog()
{
  std::vector<Node::Node> nodes;
  while (peek().has_value())
  {
    if (auto node = parse_node())
      nodes.push_back(node.value());

    else
    {
      Utils::panic("Only top level constructs are declarations");
      break;
    }
  }
  return nodes;
}

std::optional<Node::Node> Parser::parse_node()
{
  if (auto stmt = parse_stmt())
    return Node::Node{ stmt.value() };

  else if (auto expr = parse_expr())
    return Node::Node{ expr.value() };

  else if (auto decl = parse_decl())
    return Node::Node{ decl.value() };

  return {};
}

bool Parser::is_right_assoc(const BinOp& binop)
{
  switch (binop)
  {
    case BinOp::ASG:
    case BinOp::ASG_ADD:
    case BinOp::ASG_SUB:
    case BinOp::ASG_MUL:
    case BinOp::ASG_DIV:
      return true;
    default:
      return false;
  }
}

bool Parser::is_right_assoc(const UnOp& unop)
{
  return true;
}

std::optional<UnOp> Parser::parse_un_op()
{
  auto tok = peek();
  if (!tok.has_value() || !std::holds_alternative<TokenTypes::Symbol>(tok->type))
    return std::nullopt;

  else if (tok == TokenTypes::Symbol::AMP)
  {
    consume();
    return UnOp::ADDR_OF;
  }
  else if (tok == TokenTypes::Symbol::AST)
  {
    consume();
    return UnOp::DEREF;
  }

  if (tok == TokenTypes::Symbol::INC)
  {
    consume();
    return UnOp::INC;
  }
  if (tok == TokenTypes::Symbol::DEC)
  {
    consume();
    return UnOp::DEC;
  }
  if (tok == TokenTypes::Symbol::NOT)
  {
    consume();
    return UnOp::NOT;
  }
  if (tok == TokenTypes::Symbol::SUB)
  {
    consume();
    return UnOp::NEG;
  }
  return {};

}

std::optional<BinOp> Parser::parse_bin_op()
{
  auto tok1 = peek();
  if (!tok1.has_value() ||
      !std::holds_alternative<TokenTypes::Symbol>(tok1->type))
    return {};

  if (tok1 == TokenTypes::Symbol::AMP)
  {
    auto tok2 = peek(1);
    if (tok2.has_value() && tok2 == TokenTypes::Symbol::AMP)
    {
      consume();
      consume();
      return BinOp::AND;
    }
    else
      return {};
  }
  if (tok1 == TokenTypes::Symbol::GT)
  {
    auto tok2 = peek(1);
    if (tok2.has_value() && tok2 == TokenTypes::Symbol::GT)
    {
      consume();
      consume();
      return BinOp::RSH;
    }
    else
    {
      consume();
      return BinOp::GT;
    }
  }
  else if (tok1 == TokenTypes::Symbol::GTE)
  {
    consume();
    return BinOp::GTE;
  }
  else if (tok1 == TokenTypes::Symbol::LT)
  {
    auto tok2 = peek(1);
    if (tok2.has_value() && tok2 == TokenTypes::Symbol::LT)
    {
      consume();
      consume();
      return BinOp::LSH;
    }
    else
    {
      consume();
      return BinOp::LT;
    }
  }
  else if (tok1 == TokenTypes::Symbol::LTE)
  {
    consume();
    return BinOp::LTE;
  }
  else if (tok1 == TokenTypes::Symbol::EQ)
  {
    consume();
    return BinOp::EQ;
  }
  else if (tok1 == TokenTypes::Symbol::NEQ)
  {
    consume();
    return BinOp::NEQ;
  }
  else if (tok1 == TokenTypes::Symbol::ADD)
  {
    auto tok2 = peek(1);
    if (tok2.has_value() && tok2 == TokenTypes::Symbol::AEQ)
    {
      consume();
      consume();
      return BinOp::ASG_ADD;
    }
    else
    {
      consume();
      return BinOp::ADD;
    }
  }
  else if (tok1 == TokenTypes::Symbol::SUB)
  {
    auto tok2 = peek(1);
    if (tok2.has_value() && tok2 == TokenTypes::Symbol::AEQ)
    {
      consume();
      consume();
      return BinOp::ASG_SUB;
    }
    else
    {
      consume();
      return BinOp::SUB;
    }
  }
  else if (tok1 == TokenTypes::Symbol::AST)
  {
    auto tok2 = peek(1);
    if (tok2.has_value() && tok2 == TokenTypes::Symbol::AEQ)
    {
      consume();
      consume();
      return BinOp::ASG_MUL;
    }
    else
    {
      consume();
      return BinOp::MUL;
    }
  }
  else if (tok1 == TokenTypes::Symbol::SLSH)
  {
    auto tok2 = peek(1);
    if (tok2.has_value() && tok2 == TokenTypes::Symbol::AEQ)
    {
      consume();
      consume();
      return BinOp::ASG_DIV;
    }
    else
    {
      consume();
      return BinOp::DIV;
    }
  }
  else if (tok1 == TokenTypes::Symbol::AEQ)
  {
    consume();
    return BinOp::ASG;
  }
  return {};
}

std::optional<std::shared_ptr<Type>> Parser::parse_type()
{
  auto tok = peek();
  if (!tok.has_value())
    return {};

  if (*tok == TokenTypes::Literal::IDENT)
    if (auto btype = Type::string_to_base_t(tok->value.value()))
    {
      consume();
      auto tp = std::make_shared<Type>(*btype);

      for (; peek() && peek().value() == TokenTypes::Symbol::AST; consume())
        tp = std::make_shared<Type>(Type::Ptr(tp));

      return tp;
    }
  return {};
}

std::optional<Node::Ident> Parser::parse_lit_ident()
{
  if (peek() && *peek() == TokenTypes::Literal::IDENT)
    return Node::Ident(consume().value.value());
  return {};
}

std::optional<Node::Int> Parser::parse_lit_int()
{
  if (peek() && *peek() == TokenTypes::Literal::INT)
    return Node::Int(std::stoi(consume().value.value()));
  return {};
}

std::optional<std::shared_ptr<Node::Lit>> Parser::parse_lit()
{
  if (auto int_ = parse_lit_int())
    return std::make_shared<Node::Int>(*int_);
  else if (auto ident = parse_lit_ident())
    return std::make_shared<Node::Ident>(*ident);
  return {};
}

std::optional<Node::ExprStmt> Parser::parse_expr_stmt()
{
  if (auto expr = parse_expr())
  {
    consume(); // ;
    return Node::ExprStmt(expr.value());
  }
  return {};
}

std::optional<std::shared_ptr<Node::Expr>>
Parser::parse_expr(std::optional<std::shared_ptr<Node::Expr>> lhs, std::optional<BinOp> prev_op)
{
  if (auto bin_expr = parse_bin_expr(lhs, prev_op))
  {
    return bin_expr.value();
  }
  return lhs;
}

std::optional<std::shared_ptr<Node::Expr>>
Parser::parse_un_expr()
{
  if (peek() && peek().value() == TokenTypes::Symbol::AMP)
  {
    consume();
    if (auto un_expr = parse_un_expr())
      return std::make_shared<Node::UnExpr>(un_expr.value(), UnOp::ADDR_OF, true);

    Utils::panic("While parsing UnExpr, couldn't find operand");
    return {};
  }

  else if (peek() && peek().value() == TokenTypes::Symbol::AST)
  {
    consume();
    if (auto un_expr = parse_un_expr())
      return std::make_shared<Node::UnExpr>(un_expr.value(), UnOp::DEREF, true);

    Utils::panic("While parsing UnExpr, couldn't find operand");
    return {};
  }
  else if (peek() && peek().value() == TokenTypes::Symbol::INC)
  {
    consume();
    // consume();

    if (auto un_expr = parse_un_expr())
      return std::make_shared<Node::UnExpr>(un_expr.value(), UnOp::INC, true);

    Utils::panic("While parsing UnExpr, couldn't find operand");
    return {};
  }
  else if (peek() && peek().value() == TokenTypes::Symbol::DEC)
  {
    consume();
    // consume();

    if (auto un_expr = parse_un_expr())
      return std::make_shared<Node::UnExpr>(un_expr.value(), UnOp::DEC, true);

    Utils::panic("While parsing UnExpr, couldn't find operand");
    return {};
  }
  
  std::optional<std::shared_ptr<Node::Expr>> lit;
  if (auto plit = parse_lit())
    lit = *plit;
  else if (auto pexpr = parse_paren_expr())
    lit = *pexpr;
  else
    return {};

  while (true)
  {
    if (peek() && peek().value() == TokenTypes::Symbol::INC)
    {
      consume();
      // consume();
      lit = std::make_shared<Node::UnExpr>(lit.value(), UnOp::INC, false);
    }
    else if (peek() && peek().value() == TokenTypes::Symbol::DEC)
    {
      consume();
      // consume();
      lit = std::make_shared<Node::UnExpr>(lit.value(), UnOp::DEC, false);
    }
    else if (peek() && *peek() == TokenTypes::Symbol::LBRA)
    {
      consume();
      auto expr = parse_expr();
      assert(expr.has_value());

      lit = std::make_shared<Node::BinExpr>(lit.value(), BinOp::IDX, expr.value());
      assert(lit.has_value() && "idx of operator expected expression");

      assert(peek() && *peek() == TokenTypes::Symbol::RBRA);
      consume();
    }
    else
      break;
  }

  return lit;
}

std::optional<std::shared_ptr<Node::Expr>> Parser::parse_paren_expr()
{
  if (peek() && peek().value() == TokenTypes::Symbol::LPAR)
  {
    consume();
    auto expr = parse_expr();
    assert(peek() && peek().value() == TokenTypes::Symbol::RPAR && "Expected RPAR at the end of the expr");
    consume();
    return expr;
  }
  return {};
}

std::optional<std::shared_ptr<Node::Expr>>
Parser::parse_bin_expr(std::optional<std::shared_ptr<Node::Expr>> lhs, std::optional<BinOp> prev_op)
{
  const int prev_prec = (prev_op.has_value() ? precedence(prev_op.value()) : 0);
  std::optional<std::shared_ptr<Node::Expr>> rhs;
  if (auto p_expr = parse_paren_expr())
    rhs = p_expr.value();

  else if (auto call = parse_call_expr())
    rhs = std::make_shared<Node::Call>(call.value());

  else
    rhs = parse_un_expr();
    // rhs = parse_lit();

  if (!rhs.has_value())
  {
    if (lhs.has_value())
      Utils::panic("Expected an expression");

    return {};
  }

  auto next_op = parse_bin_op();
  if (!next_op.has_value())
  {
    auto ret = (
      !lhs.has_value() ?
      rhs :
      std::make_shared<Node::BinExpr>(lhs.value(), prev_op.value(), rhs.value())
    );
    // ret = parse_un_expr();
    return ret;
  }

  int next_prec = precedence(next_op.value());
  if (
    next_prec > prev_prec ||
    (next_op.value() == prev_op && is_right_assoc(next_op.value()))
  )
  {
    auto expr = parse_expr(rhs.value(), next_op);
    assert(expr.has_value() && "Must be true");

    if (!prev_op)
      return expr;

    Node::BinExpr res(lhs.value(), prev_op.value(), expr.value());
    std::optional<std::shared_ptr<Node::Expr>> ret = std::make_shared<Node::BinExpr>(res);
    // ret = parse_un_expr(&ret);
    return ret;
  }
  else
  {
    Node::BinExpr new_lhs(lhs.value(), prev_op.value(), rhs.value());
    auto ret = parse_expr(std::make_shared<Node::BinExpr>(new_lhs), next_op);
    // ret = parse_un_expr(&ret);
    return ret;
  }
}

std::optional<Node::Call> Parser::parse_call_expr()
{
  if (!peek() || peek().value() != TokenTypes::Literal::IDENT || !peek(1) ||
      peek(1).value() != TokenTypes::Symbol::LPAR)
    return {};

  Node::Call call(*consume().value);
  if (auto arg_list = parse_arg_list())
  {
    call.args = arg_list.value();
  }
  return call;
}

std::optional<Node::If> Parser::parse_if_stmt()
{
  auto tok = peek();
  if (!tok.has_value() || tok != TokenTypes::Keyword::IF)
    return std::nullopt;

  consume();
  // Mandatory '(' expr ')'
  consume();
  auto cond = parse_expr();
  if (!cond.has_value())
    return std::nullopt;
  consume();

  // Mandatory '{' scope '}' (for now)
  auto scope = parse_scope();
  if (!scope.has_value())
    return std::nullopt;

  auto tok1 = peek();
  if (tok1.has_value() && tok1 == TokenTypes::Keyword::ELSE)
  {
    consume();
    auto elif = parse_if_stmt();
    if (!elif.has_value())
    {
      auto else_scope = parse_scope();
      if (!else_scope.has_value())
      {
        Utils::panic("Expected an else scope");
        return std::nullopt;
      }
      auto true_cond = std::make_shared<Node::Int>(Node::Int(1));
      auto else_node = Node::If(
          true_cond, else_scope.value(), std::nullopt);
      auto res = Node::If(cond.value(), scope.value(), else_node);
      return res;
    }
    else
    {
      return Node::If(cond.value(), scope.value(), elif.value());
    }
  }
  return Node::If(cond.value(), scope.value(), std::nullopt);
  // return std::nullopt;
}

std::optional<Node::Import> Parser::parse_import()
{
  if (!peek().has_value() || peek().value() != TokenTypes::Keyword::IMPORT)
    return std::nullopt;

  consume();
  Node::Import import;
  while (peek().has_value() && peek().value() == TokenTypes::Literal::IDENT)
  {
    if (
      peek().has_value() && peek().value() == TokenTypes::Symbol::COL &&
      peek(1).has_value() && peek(1).value() == TokenTypes::Symbol::COL
      )
    {
      import.mod.emplace_back(consume().value.value());
      consume();
      consume();
    }
    else if (peek().has_value() && peek().value() == TokenTypes::Symbol::SEMICOL)
    {
      import.mod.emplace_back(consume().value.value());
      consume();
      break;
    }
    else
      Utils::panic("Incorrect module import format");
  }
  return import;
}

std::optional<Node::Scope> Parser::parse_scope()
{
  if (!peek() || peek().value() != TokenTypes::Symbol::LCUR)
    return {};

  consume();
  Node::Scope scope;
  while (peek().has_value() && peek().value() != TokenTypes::Symbol::RCUR)
  {
    if (auto stmt = parse_stmt())
    {
      scope.push_back(stmt.value());
    }
    else if (auto expr = parse_expr())
    {
      scope.push_back(std::dynamic_pointer_cast<Node::Expr>(expr.value()));
      consume(); // ;
    }
    else
    {
      Utils::panic("Expected statements in scope");
      return {};
    }
  }
  consume();
  return scope;
}

std::optional<std::vector<std::shared_ptr<Node::Expr>>> Parser::parse_arg_list()
{
  std::vector<std::shared_ptr<Node::Expr>> args;
  consume();
  while (auto expr = parse_expr())
  {
    args.emplace_back(expr.value());
    if (peek() && peek().value() == TokenTypes::Symbol::COM)
      consume();
    else
      break;
  }
  consume();
  return args;
}

std::optional<std::vector<Node::Param>> Parser::parse_param_list()
{
  std::vector<Node::Param> params;
  consume();
  while (peek() && peek().value() == TokenTypes::Literal::IDENT && peek(1) &&
         peek(1).value() == TokenTypes::Literal::IDENT)
  {
    auto type = parse_type();
    std::string name_str = *consume().value;

    params.emplace_back(**type, std::move(name_str));
    if (peek() && peek().value() == TokenTypes::Symbol::COM)
      consume();
    else
      break;
  }
  consume();
  return params;
}

std::optional<Node::Struct> Parser::parse_struct()
{
  if (!peek() || peek().value() != TokenTypes::Keyword::STRUCT)
    return std::nullopt;

  consume();

  Node::Struct struct_node(parse_lit_ident().value());
  consume(); // {
  while (peek() && peek().value() != TokenTypes::Symbol::RCUR)
  {
    auto t1 = consume();
    auto t2 = consume();

    assert(t1 == TokenTypes::Literal::IDENT && t2 == TokenTypes::Literal::IDENT);
    struct_node.members.emplace_back(t1.value.value(), t2.value.value());

    consume(); // ;
  }
  consume(); // }
  return struct_node;
}

std::optional<Node::Func> Parser::parse_func()
{
  if (!peek() || peek().value() != TokenTypes::Keyword::FN)
    return {};

  consume();
  std::string name;
  if (peek() && peek().value() == TokenTypes::Literal::IDENT)
  {
    name = *peek()->value;
    consume();
  }

  auto ident = Node::Ident(name);
  std::vector<Node::Param> params;
  Type ret_t = Type(Type::Base::VOID);
  if (auto param_list = parse_param_list())
    params = param_list.value();

  // RARROW (->)
  if (peek() && peek().value() == TokenTypes::Symbol::SUB && peek(1) &&
      peek(1).value() == TokenTypes::Symbol::GT)
  {
    consume();
    consume();
    const auto type = parse_type();
    assert(type.has_value());
    ret_t = **type;
  }

  Node::Func func(std::move(ident), ret_t);
  func.params = params;
  if (peek() && peek().value() == TokenTypes::Symbol::SEMICOL)
  {
    consume();
    return func;
  }

  if (auto scope = parse_scope())
    func.scope = scope.value();

  return func;
}

std::optional<Node::For> Parser::parse_for_stmt()
{
  if (!peek() || peek().value() != TokenTypes::Keyword::FOR)
    return std::nullopt;

  consume();
  Node::For loop;

  consume(); // (
  if (auto asgn = parse_asgn_stmt())
    loop.init = std::make_shared<Node::Asgn>(asgn.value());
  else
    consume(); // ;
  // consume(); // ;

  if (auto cond = parse_expr())
    loop.cond = cond.value();
  consume(); // ;

  if (auto adv = parse_expr())
    loop.adv = adv.value();

  consume(); // )
  if (auto scope = parse_scope())
    loop.scope = std::make_shared<Node::Scope>(scope.value());

  else
    Utils::panic("Expected scope in for");

  return loop;
}

std::optional<std::shared_ptr<Node::Decl>> Parser::parse_decl()
{
  if (peek() && peek().value() == TokenTypes::Keyword::FN)
    return std::make_shared<Node::Func>(parse_func().value());
  return std::nullopt;
}

std::optional<Node::Ret> Parser::parse_ret_stmt()
{
  if (!peek() || peek().value() != TokenTypes::Keyword::RET)
    return std::nullopt;

  consume();
  Node::Ret ret(parse_expr());
  consume(); // ;
  return ret;
}


void Parser::parse_arr(std::shared_ptr<Type>& type)
{
  while (peek() && *peek() == TokenTypes::Symbol::LBRA)
  {
    consume();
    if (auto expr = parse_expr())
    {
      type->type = Type::Arr(
        std::make_shared<Type>(*type),
        std::move(*expr)
      );
    }
    else
      assert(false && "L Syntax");

    assert(peek() && *peek() == TokenTypes::Symbol::RBRA);
    consume();
  }
}

std::optional<Node::Asgn> Parser::parse_asgn_stmt()
{
  if (auto t = parse_type())
  {
    auto ident = consume();
    if (peek() == TokenTypes::Symbol::LBRA)
      parse_arr(*t);

    if (peek() && peek() == TokenTypes::Symbol::AEQ)
    {
      consume();
      if (auto expr = parse_expr())
      {
        consume(); // ;
        return Node::Asgn(t.value(), ident.value.value(), expr.value());
      }
      else if (auto decl = parse_decl())
      {
        Utils::panic("Not implemented");
        return {};
      }
      else
        return {};
    }
    else
    {
      consume(); // ;
      return Node::Asgn(t.value(), ident.value.value());
    }
  }
  return {};
}

std::optional<Node::Continue> Parser::parse_continue_stmt()
{
  if (peek() && *peek() == TokenTypes::Keyword::CONTINUE)
  {
    consume();
    assert(peek() && *peek() == TokenTypes::Symbol::SEMICOL);
    consume();
    return Node::Continue();
  }
  return {};
}

std::optional<Node::Break> Parser::parse_break_stmt()
{
  if (peek() && *peek() == TokenTypes::Keyword::BREAK)
  {
    consume();
    assert(peek() && *peek() == TokenTypes::Symbol::SEMICOL);
    consume();
    return Node::Break();
  }
  return {};
}

std::optional<std::shared_ptr<Node::Stmt>> Parser::parse_stmt()
{
  if (auto asgn_stmt = parse_asgn_stmt())
  {
    return std::make_shared<Node::Asgn>(asgn_stmt.value());
  }
  else if (auto expr_stmt = parse_expr_stmt())
  {
    return std::make_shared<Node::ExprStmt>(expr_stmt.value());
  }
  /*
  else if (auto reasgn_stmt = parse_reasgn_stmt())
  {
    return std::make_shared<Node::ReAsgn>(reasgn_stmt.value());
  }
  */
  else if (auto if_stmt = parse_if_stmt())
  {
    return std::make_shared<Node::If>(if_stmt.value());
  }
  else if (auto for_stmt = parse_for_stmt())
  {
    return std::make_shared<Node::For>(for_stmt.value());
  }
  else if (auto cont_stmt = parse_continue_stmt())
  {
    return std::make_shared<Node::Continue>(cont_stmt.value());
  }
  else if (auto brk_stmt = parse_break_stmt())
  {
    return std::make_shared<Node::Break>(brk_stmt.value());
  }
  else if (auto ret_stmt = parse_ret_stmt())
  {
    return std::make_shared<Node::Ret>(ret_stmt.value());
  }
  else if (auto imp_stmt = parse_import())
  {
    return std::make_shared<Node::Import>(imp_stmt.value());
  }
  return {};
}

[[nodiscard]] std::optional<Token> Parser::peek(const int offset)
{
  if (m_index + offset < m_tokens.size())
    return m_tokens[m_index + offset];
  return {};
}

Token Parser::consume() { return m_tokens[m_index++]; }
