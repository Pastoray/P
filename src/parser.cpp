#include "parser.hpp"
#include "tokenizer.hpp"
#include "utils.hpp"

#include <cassert>
#include <memory>
#include <optional>
#include <variant>

uint32_t Node::Ident::nid = 0;
Parser::Parser(std::vector<Token>& tokens)
  : m_tokens(std::move(tokens)),
    m_index(0) {}

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

std::optional<UnOp> Parser::parse_pref_un_op()
{
  auto tok = peek();
  if (!tok.has_value() || !std::holds_alternative<TokenTypes::Symbol>(tok->type))
    return {};

  if (tok == TokenTypes::Symbol::AMP)
  {
    consume();
    return UnOp::ADDR_OF;
  }
  if (tok == TokenTypes::Symbol::AST)
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

std::optional<UnOp> Parser::parse_suff_un_op()
{
  auto tok = peek();
  if (!tok.has_value() || !std::holds_alternative<TokenTypes::Symbol>(tok->type))
    return {};

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
  else if (tok1 == TokenTypes::Symbol::DOT)
  {
    consume();
    return BinOp::DOT;
  }
  else if (tok1 == TokenTypes::Symbol::ARROW)
  {
    consume();
    return BinOp::ARROW;
  }
  return {};
}

std::optional<Node::TypeRef> Parser::parse_type_ref()
{
  auto tok = peek();
  if (!tok.has_value() || !tok->value.has_value() || !m_tracker.is_type(tok->value.value()))
    return {};

  consume();
  auto tp = m_tracker.get_type(tok->value.value());
  for (; peek() && peek().value() == TokenTypes::Symbol::AST; consume())
    tp = std::make_shared<Type>(Type::Ptr(tp));

  return Node::TypeRef(tp);
}

std::optional<Node::TypeDef> Parser::parse_type_def()
{
  if (auto stc = parse_struct())
    return Node::TypeDef(std::make_shared<Node::Struct>(stc.value()), m_tracker.get_type(stc.value().id.name));
  else if (auto uni = parse_union())
    return Node::TypeDef(std::make_shared<Node::Union>(uni.value()), m_tracker.get_type(uni.value().id.name));
  else if (auto enm = parse_enum())
    return Node::TypeDef(std::make_shared<Node::Enum>(enm.value()), m_tracker.get_type(enm.value().id.name));
  return {};
}

std::optional<std::variant<Node::TypeDef, Node::TypeRef>> Parser::parse_type()
{
  if (auto def = parse_type_def())
    return def;
  else if (auto ref = parse_type_ref())
    return ref;
  return {};
}

std::optional<Node::Path> Parser::parse_lit_path()
{
  RollbackGuard guard(m_index);
  std::vector<Node::Ident> res;
  while (auto ident = parse_lit_ident())
  {
    res.push_back(ident.value());
    if (peek() && peek() == TokenTypes::Symbol::NSR) consume();
    else break;
  }

  // If it's 1 it couldve been parsed as a standalone ident
  if (res.size() <= 1)
    return {};

  guard.commit();
  return Node::Path(res);
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

std::optional<Node::Double> Parser::parse_lit_double()
{
  if (peek() && *peek() == TokenTypes::Literal::DOUBLE)
    return Node::Double(std::stod(consume().value.value()));
  return {};
}

std::optional<Node::String> Parser::parse_lit_string()
{
  if (peek() && *peek() == TokenTypes::Literal::STRING)
    return Node::String(consume().value.value());
  return {};
}

std::optional<Node::Char> Parser::parse_lit_char()
{
  if (peek() && *peek() == TokenTypes::Literal::CHAR)
    return Node::Char(consume().value.value());
  return {};
}

std::optional<std::shared_ptr<Node::Lit>> Parser::parse_lit()
{
  if (auto int_ = parse_lit_int())
    return std::make_shared<Node::Int>(*int_);
  if (auto double_ = parse_lit_double())
    return std::make_shared<Node::Double>(*double_);
  if (auto string = parse_lit_string())
    return std::make_shared<Node::String>(*string);
  if (auto c = parse_lit_char())
    return std::make_shared<Node::Char>(*c);
  if (auto path = parse_lit_path())
    return std::make_shared<Node::Path>(*path);
  if (auto ident = parse_lit_ident())
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
  auto parse_paren_expr = [&]() -> std::optional<std::shared_ptr<Node::Expr>>
  {
    RollbackGuard guard(m_index);
    if (!peek() || peek() != TokenTypes::Symbol::LPAR) return {};

    consume();
    auto expr = parse_expr();
    if (!peek() || peek() != TokenTypes::Symbol::RPAR) return {};
    consume();
    guard.commit();
    return expr;
  };

  auto precedence = [&](const std::variant<UnOp, BinOp>& op)
  {
    return std::visit(
      Utils::overloaded
      {
        [&](const UnOp& uop) { return ::precedence(uop); },
        [&](const BinOp& bop) { return ::precedence(bop); }
      }, op
    );
  };

  std::vector<UnOp> pref;
  std::vector<std::variant<UnOp, BinOp>> suff;
  std::vector<std::vector<std::shared_ptr<Node::Expr>>> post_exprs;
  {
    int cp = 0;
    for (auto uop = parse_pref_un_op(); uop; cp = precedence(uop.value()), uop = parse_pref_un_op())
    {
      assert(cp <= precedence(uop.value()));
      pref.emplace_back(uop.value());
    }
  }

  std::shared_ptr<Node::Expr> node;
  if (auto lit = parse_lit()) node = lit.value();
  else if (auto pexpr = parse_paren_expr()) node = pexpr.value();
  else
  {
    assert(pref.empty() && suff.empty() && !prev_op);
    return {};
  }

  {
    int cp = 1000;
    while (true)
    {
      for (auto uop = parse_suff_un_op(); uop; cp = precedence(uop.value()), uop = parse_suff_un_op())
      {
        assert(cp >= precedence(uop.value()));
        suff.emplace_back(uop.value());
      }
      if (peek() && peek() == TokenTypes::Symbol::LBRA)
      {
        consume();
        auto expr = parse_expr();
        assert(expr);
        assert(cp >= precedence(BinOp::IDX));

        suff.emplace_back(BinOp::IDX);
        post_exprs.push_back({ expr.value() });

        cp = precedence(BinOp::IDX);
        consume();
        continue;
      }
      if (peek() && peek() == TokenTypes::Symbol::LPAR)
      {
        consume();
        auto expr = parse_expr();
        assert(expr);
        assert(cp >= precedence(BinOp::CALL));

        suff.emplace_back(BinOp::CALL);
        post_exprs.push_back({ expr.value() });

        while (peek() && peek() == TokenTypes::Symbol::COM)
        {
          consume();
          auto nexpr = parse_expr();
          assert(nexpr);
          post_exprs.back().push_back(nexpr.value());
        }

        cp = precedence(BinOp::CALL);
        assert(peek() && peek() == TokenTypes::Symbol::RPAR);
        consume();

        continue;
      }
      break;
    }

  }

  auto curr_midx = m_index;
  auto curr_op = parse_bin_op();
  if (!pref.empty() && prev_op) assert(precedence(pref.back()) > precedence(prev_op.value()));
  if (!suff.empty() && curr_op) assert(precedence(suff[0]) > precedence(curr_op.value()));

  std::reverse(suff.begin(), suff.end());
  std::reverse(post_exprs.begin(), post_exprs.end());
  while (!pref.empty() || !suff.empty())
  {
    auto p = pref.empty() ? 0 : precedence(pref.back());
    auto s = suff.empty() ? 0 : precedence(suff.back());
    if (p >= s)
    {
      if (curr_op && p < precedence(curr_op.value())) break;
      node = std::make_shared<Node::UnExpr>(node, pref.back(), true);
      pref.pop_back();
    }
    else
    {
      if (prev_op && s < precedence(prev_op.value())) break;
      auto op = std::get_if<BinOp>(&suff.back());
      if (op && *op == BinOp::CALL)
      { node = std::make_shared<Node::Call>(node, post_exprs.back()); post_exprs.pop_back(); }
      else if (op)
      { node = std::make_shared<Node::BinExpr>(node, *op, post_exprs.back().back()); post_exprs.pop_back(); }
      else node = std::make_shared<Node::UnExpr>(node, std::get<UnOp>(suff.back()), false);
      suff.pop_back();
    }
  }
  if (!prev_op && !curr_op) return node;

  std::shared_ptr<Node::Expr> res;
  if (!curr_op || (prev_op && precedence(prev_op.value()) >= precedence(curr_op.value())))
  {
    res = std::make_shared<Node::BinExpr>(lhs.value(), prev_op.value(), node);
    for (; !suff.empty(); suff.pop_back())
    {
      auto op = std::get_if<BinOp>(&suff.back());
      if (op && *op == BinOp::CALL)
      { res = std::make_shared<Node::Call>(res, post_exprs.back()); post_exprs.pop_back(); }
      else if (op)
      { node = std::make_shared<Node::BinExpr>(res, *op, post_exprs.back().back()); post_exprs.pop_back(); }
      else res = std::make_shared<Node::UnExpr>(res, std::get<UnOp>(suff.back()), false);
    }

    m_index = curr_midx;
    return res;
  }
  
  while (curr_op && (!prev_op || precedence(prev_op.value()) < precedence(curr_op.value())))
  {
    node = parse_bin_expr(node, curr_op).value();
    for (; !pref.empty(); pref.pop_back())
      node = std::make_shared<Node::UnExpr>(node, pref.back(), true);

    curr_midx = m_index;
    curr_op = parse_bin_op();
  }

  if (curr_op) m_index = curr_midx;
 
  if (!lhs) res = node;
  else res = std::make_shared<Node::BinExpr>(lhs.value(), prev_op.value(), node);
  return res;
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
  m_tracker.push_scope();
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
    else if (auto decl = parse_decl())
    {
      scope.push_back(decl.value());
    }
    else
    {
      Utils::panic("Expected statements in scope");
      return {};
    }
  }
  m_tracker.pop_scope();
  consume();
  return scope;
}

std::optional<std::vector<std::shared_ptr<Node::Expr>>> Parser::parse_arg_list()
{
  RollbackGuard guard(m_index);
  if (!peek() || peek() != TokenTypes::Symbol::LPAR) return {};

  consume();
 
  std::vector<std::shared_ptr<Node::Expr>> args;
  while (auto expr = parse_expr())
  {
    args.emplace_back(expr.value());
    if (peek() && peek() == TokenTypes::Symbol::COM) consume();
    else
      break;
  }
  if (!peek() || peek() != TokenTypes::Symbol::RPAR) return {};
  consume();

  guard.commit();
  return args;
}

std::optional<std::vector<Node::Param>> Parser::parse_param_list()
{
  RollbackGuard guard(m_index);
  std::vector<Node::Param> params;
  if (!peek() || peek() != TokenTypes::Symbol::LPAR) return {};

  consume();
  while (auto type = parse_type_ref())
  {
    std::string name_str = *consume().value;

    params.emplace_back(*type, std::move(name_str));
    if (peek() && peek().value() == TokenTypes::Symbol::COM) consume();
    else break;
  }

  if (!peek() || peek() != TokenTypes::Symbol::RPAR) return {};
  consume();

  guard.commit();
  return params;
}

std::optional<Node::Struct> Parser::parse_struct()
{
  if (!peek() || peek().value() != TokenTypes::Keyword::STRUCT)
    return {};

  consume();

  auto idt = parse_lit_ident();
  assert(idt.has_value());
  Node::Struct struct_node(idt.value());
  auto type = std::make_shared<Type>(Type::Struct(struct_node.id.name));
  auto& stype = std::get<Type::Struct>(type->type);
  m_tracker.push_type(struct_node.id.name, type);
  consume(); // {
  while (peek() && peek().value() != TokenTypes::Symbol::RCUR)
  {
    if (auto tp = parse_type())
    {
      assert(peek() && peek() == TokenTypes::Literal::IDENT);
      auto mem = Node::Ident(consume().value.value());
      struct_node.members.emplace_back(tp.value(), mem);
      
      assert(peek() && peek() == TokenTypes::Symbol::SEMICOL);
      consume(); // ;
    }
    else
    {
      std::string msg = "Expected something in decl scope, got: ";
      if (peek())
        Utils::panic(msg + Token::to_string(peek().value()));
      else
        Utils::panic(msg + std::string("Nothing"));
    }
  }
  consume(); // }
  return struct_node;
}

std::optional<Node::Union> Parser::parse_union()
{
  if (!peek() || peek().value() != TokenTypes::Keyword::UNION)
    return {};

  consume();

  auto idt = parse_lit_ident();
  assert(idt.has_value());
  Node::Union union_node(idt.value());
  auto type = std::make_shared<Type>(Type::Union(union_node.id.name));
  m_tracker.push_type(union_node.id.name, type);
  consume(); // {
  while (peek() && peek().value() != TokenTypes::Symbol::RCUR)
  {
    if (auto tp = parse_type())
    {
      assert(peek() && peek() == TokenTypes::Literal::IDENT);
      union_node.members.emplace_back(
        tp.value(), Node::Ident(consume().value.value())
      );
      assert(peek() && peek() == TokenTypes::Symbol::SEMICOL);
      consume(); // ;
    }
    else
    {
      std::string msg = "Expected something in decl scope, got: ";
      if (peek())
        Utils::panic(msg + Token::to_string(peek().value()));
      else
        Utils::panic(msg + std::string("Nothing"));
    }
  }
  consume(); // }
  return union_node;
}

std::optional<Node::Namespace> Parser::parse_namespace()
{
  if (!peek() || peek() != TokenTypes::Keyword::NAMEPSACE)
    return {};

  consume();

  auto idt = parse_lit_ident();
  assert(idt.has_value());
  Node::Namespace ns_node(idt.value());

  if (auto scp = parse_scope())
    ns_node.scp = scp.value();
  else
    assert(false && "Namespace scope is not optional");

  return ns_node;
}

std::optional<Node::Enum> Parser::parse_enum()
{
  if (!peek() || peek().value() != TokenTypes::Keyword::ENUM)
    return {};

  consume();
  auto tok = consume();

  Node::Enum enm(tok.value.value());
  std::shared_ptr<Node::Expr> prev = std::make_shared<Node::Int>(-1);
  assert(peek() && peek() == TokenTypes::Symbol::LCUR);
  consume(); // {
  while (peek() && peek().value() != TokenTypes::Symbol::RCUR)
  {
    auto en = consume();
    assert(en == TokenTypes::Literal::IDENT);

    if (peek() && peek() == TokenTypes::Symbol::AEQ)
    {
      consume();
      if (auto expr = parse_expr())
      {
        prev = expr.value();
        auto asgn = Node::Asgn(
          Node::TypeRef(std::make_shared<Type>(Type::Base::I32)),
          en.value.value(),
          expr.value()
        );
        enm.enums.push_back(asgn);
      }
      else assert(false && "Expected expression");
    }
    else
    {
      prev = std::make_shared<Node::BinExpr>(prev, BinOp::ADD, std::make_shared<Node::Int>(1));
      auto asgn = Node::Asgn(
        Node::TypeRef(std::make_shared<Type>(Type::Base::I32)),
        en.value.value(),
        prev
      );
      enm.enums.push_back(asgn);
    }
    if (peek() && peek() == TokenTypes::Symbol::COM)
      consume(); // ,
    else
    {
      assert(peek() && peek() == TokenTypes::Symbol::RCUR);
      break;
    }
  }
  consume(); // }
  auto type = std::make_shared<Type>(Type::Enum(enm.id.name));
  m_tracker.push_type(enm.id.name, type);
  return enm;
}

std::optional<Node::Func> Parser::parse_func()
{
  RollbackGuard guard(m_index);
  if (!peek() || peek().value() != TokenTypes::Keyword::FN)
    return {};

  consume();
  std::string name;
  if (!peek() || peek() != TokenTypes::Literal::IDENT)
    return {};

  name = *consume().value;

  auto ident = Node::Ident(name);
  std::vector<Node::Param> params;
  auto ret_t = Node::TypeRef(std::make_shared<Type>(Type::Base::VOID));
  if (auto param_list = parse_param_list())
    params = param_list.value();
  else
    return {};

  // RARROW (->)
  if (peek() && peek().value() == TokenTypes::Symbol::ARROW)
  {
    consume();
    const auto type = parse_type_ref();
    if (!type)
      return {};

    ret_t = *type;
  }
  else return {};

  Node::Func func(std::move(ident), ret_t);
  func.params = params;
  if (peek() && peek().value() == TokenTypes::Symbol::SEMICOL)
  {
    consume();
    guard.commit();
    return func;
  }

  if (auto scope = parse_scope())
    func.scope = scope.value();

  guard.commit();
  return func;
}

std::optional<Node::For> Parser::parse_for_stmt()
{
  if (!peek() || peek().value() != TokenTypes::Keyword::FOR)
    return {};

  consume();
  Node::For loop;

  consume(); // (
  if (auto asgn = parse_asgn_stmt())
    loop.init = std::make_shared<Node::Asgn>(asgn.value());
  else
    consume(); // ;

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
  else if (peek() && peek().value() == TokenTypes::Keyword::NAMEPSACE)
    return std::make_shared<Node::Namespace>(parse_namespace().value());
  return {};
}

std::optional<Node::Ret> Parser::parse_ret_stmt()
{
  if (!peek() || peek().value() != TokenTypes::Keyword::RET)
    return {};

  consume();
  Node::Ret ret(parse_expr());
  consume(); // ;
  return ret;
}

void Parser::parse_arr(std::variant<Node::TypeDef, Node::TypeRef>& type)
{
  auto tp = std::visit(
    Utils::overloaded
    {
      [this](Node::TypeDef& td) -> std::shared_ptr<Type> { return td.res_t; },
      [](Node::TypeRef& tr) -> std::shared_ptr<Type> { return tr.res_t; }
    }, type
  );

  while (peek() && peek() == TokenTypes::Symbol::LBRA)
  {
    consume();
    if (auto expr = parse_expr())
    {
      tp->type = Type::Arr(
        std::make_shared<Type>(*tp),
        std::move(*expr)
      );
    }
    else
      assert(false && "Bad Syntax");

    assert(peek() && peek() == TokenTypes::Symbol::RBRA);
    consume();
  }
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
      assert(false && "Bad syntax");

    assert(peek() && peek() == TokenTypes::Symbol::RBRA);
    consume();
  }
}

std::optional<Node::Asgn> Parser::parse_asgn_stmt()
{
  RollbackGuard guard(m_index);
  if (auto t = parse_type())
  {
    if (!peek() || peek() != TokenTypes::Literal::IDENT)
      return {};

    auto ident = consume();
    if (peek() == TokenTypes::Symbol::LBRA)
      parse_arr(*t);

    if (peek() && peek() == TokenTypes::Symbol::AEQ)
    {
      consume();
      if (auto expr = parse_expr())
      {
        consume(); // ;
        guard.commit();
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
      guard.commit();
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
  else if (auto type_def = parse_type_def())
  {
    auto ret = std::make_shared<Node::TypeDef>(type_def.value());
    consume();
    return ret;
  }
  else if (auto expr_stmt = parse_expr_stmt())
  {
    return std::make_shared<Node::ExprStmt>(expr_stmt.value());
  }
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

