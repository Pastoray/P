#include "parser.hpp"
#include "diagnostic.hpp"
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
  if (!tok) return {};
  if (tok == TokenTypes::Keyword::FN)
  {
    auto tok2 = peek(1);
    if (!tok2 || tok2 != TokenTypes::Symbol::LT) return {};

    consume();
    consume();
    std::vector<std::shared_ptr<Type>> params;

    if (peek() && peek() == TokenTypes::Symbol::LPAR)
    {
      consume();
      while (peek() && peek() != TokenTypes::Symbol::RPAR)
      {
        auto tp = parse_type();
        if (!tp)
        {
          Diagnostic::error(SrcLoc({}, 0, 0, {}), "Expected closing ')' inside fn<(...), ...>");
          sync();
          return {};
        }
        auto t = Node::get_res_t(*tp);
        params.push_back(t);
        if (peek() && peek() == TokenTypes::Symbol::COM) consume();
        else if (!peek() || peek() != TokenTypes::Symbol::RPAR)
        {
          Diagnostic::error(SrcLoc({}, 0, 0, {}), "Expected closing ')' inside fn<(...), ...>");
          sync();
          return {};
        }
      }
      consume();
    }
    if (!peek() || peek() != TokenTypes::Symbol::COM)
    {
      Diagnostic::error(SrcLoc({}, 0, 0, {}), "Expected ',' after pack type (...)");
      sync();
      return {};
    }
    consume();
    auto ret_tp = parse_type();
    if (!ret_tp)
    {
      Diagnostic::error(SrcLoc({}, 0, 0, {}), "Expected return type inside fn<(...), ...>");
      sync();
      return {};
    }
    auto ret_t = Node::get_res_t(*ret_tp);
    if (!peek() || peek() != TokenTypes::Symbol::GT)
    {
      Diagnostic::error(SrcLoc({}, 0, 0, {}), "Expected closing '>'");
      sync();
      return {};
    }
    consume();
    auto res_t = std::make_shared<Type>(Type::Func(params, ret_t));
    return Node::TypeRef(res_t, tok->loc);
  }
  if (!tok->value.has_value() || !m_tracker.is_type(tok->value.value()))
    return {};

  consume();
  auto tp = m_tracker.get_type(tok->value.value());
  for (; peek() && peek().value() == TokenTypes::Symbol::AST; consume())
    tp = std::make_shared<Type>(Type::Ptr(tp));

  return Node::TypeRef(tp, tok->loc);
}

std::optional<Node::TypeDef> Parser::parse_type_def()
{
  if (auto stc = parse_struct())
    return Node::TypeDef(std::make_shared<Node::Struct>(stc.value()), m_tracker.get_type(stc.value().id.name), stc->loc);
  else if (auto uni = parse_union())
    return Node::TypeDef(std::make_shared<Node::Union>(uni.value()), m_tracker.get_type(uni.value().id.name), uni->loc);
  else if (auto enm = parse_enum())
    return Node::TypeDef(std::make_shared<Node::Enum>(enm.value()), m_tracker.get_type(enm.value().id.name), enm->loc);
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
  if (peek() && peek() == TokenTypes::Literal::IDENT)
  {
    auto tok = consume();
    return Node::Ident(tok.value.value(), tok.loc);
  }
  return {};
}

std::optional<Node::Int> Parser::parse_lit_int()
{
  if (peek() && peek() == TokenTypes::Literal::INT)
  {
    auto tok = consume();
    return Node::Int(std::stoi(tok.value.value()), tok.loc);
  }
  return {};
}

std::optional<Node::Bool> Parser::parse_lit_bool()
{
  if (peek() && peek() == TokenTypes::Literal::BOOL && peek())
  {
    auto tok = consume();
    return Node::Bool(tok.value.value() == "true", tok.loc);
  }
  return {};
}

std::optional<Node::Double> Parser::parse_lit_double()
{
  if (peek() && peek() == TokenTypes::Literal::DOUBLE)
  {
    auto tok = consume();
    return Node::Double(std::stod(tok.value.value()), tok.loc);
  }
  return {};
}

std::optional<Node::String> Parser::parse_lit_string()
{
  if (peek() && peek() == TokenTypes::Literal::STRING)
  {
    auto tok = consume();
    return Node::String(tok.value.value(), tok.loc);
  }
  return {};
}

std::optional<Node::Char> Parser::parse_lit_char()
{
  if (peek() && peek() == TokenTypes::Literal::CHAR)
  {
    auto tok = consume();
    return Node::Char(tok.value.value(), tok.loc);
  }
  return {};
}

std::optional<std::shared_ptr<Node::Lit>> Parser::parse_lit()
{
  if (auto int_ = parse_lit_int())
    return std::make_shared<Node::Int>(*int_);
  if (auto double_ = parse_lit_double())
    return std::make_shared<Node::Double>(*double_);
  if (auto bool_ = parse_lit_bool())
    return std::make_shared<Node::Bool>(*bool_);
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
    if (!peek() || peek() != TokenTypes::Symbol::RPAR)
    {
      Diagnostic::error(peek()->loc, "Expected ')' at the end of the expression'\n'");
      sync();
      return std::make_shared<Node::ErrExpr>(peek()->loc);
    }
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
      node = std::make_shared<Node::UnExpr>(node, pref.back(), true, SrcLoc({}, 0, 0, {}));
      pref.pop_back();
    }
    else
    {
      if (prev_op && s < precedence(prev_op.value())) break;
      auto op = std::get_if<BinOp>(&suff.back());
      if (op && *op == BinOp::CALL)
      { node = std::make_shared<Node::Call>(node, post_exprs.back(), SrcLoc({}, 0, 0, {})); post_exprs.pop_back(); }
      else if (op)
      {
        node = std::make_shared<Node::BinExpr>(
          node,
          *op,
          post_exprs.back().back(),
          SrcLoc({}, 0, 0, {})
        );
        post_exprs.pop_back();
      }
      else node = std::make_shared<Node::UnExpr>(node, std::get<UnOp>(suff.back()), false, SrcLoc({}, 0, 0, {}));
      suff.pop_back();
    }
  }
  if (!prev_op && !curr_op) return node;

  std::shared_ptr<Node::Expr> res;
  if (!curr_op || (prev_op && precedence(prev_op.value()) >= precedence(curr_op.value())))
  {
    res = std::make_shared<Node::BinExpr>(lhs.value(), prev_op.value(), node, SrcLoc({}, 0, 0, {}));
    for (; !suff.empty(); suff.pop_back())
    {
      auto op = std::get_if<BinOp>(&suff.back());
      if (op && *op == BinOp::CALL)
      { res = std::make_shared<Node::Call>(res, post_exprs.back(), SrcLoc({}, 0, 0, {})); post_exprs.pop_back(); }
      else if (op)
      {
        node = std::make_shared<Node::BinExpr>(
          res,
          *op,
          post_exprs.back().back(),
          SrcLoc({}, 0, 0, {})
        );
        post_exprs.pop_back();
      }
      else res = std::make_shared<Node::UnExpr>(res, std::get<UnOp>(suff.back()), false, SrcLoc({}, 0, 0, {}));
    }

    m_index = curr_midx;
    return res;
  }
  
  while (curr_op && (!prev_op || precedence(prev_op.value()) < precedence(curr_op.value())))
  {
    node = parse_bin_expr(node, curr_op).value();
    for (; !pref.empty(); pref.pop_back())
      node = std::make_shared<Node::UnExpr>(node, pref.back(), true, SrcLoc({}, 0, 0, {}));

    curr_midx = m_index;
    curr_op = parse_bin_op();
  }

  if (curr_op) m_index = curr_midx;
 
  if (!lhs) res = node;
  else res = std::make_shared<Node::BinExpr>(lhs.value(), prev_op.value(), node, SrcLoc({}, 0, 0, {}));
  return res;
}

std::optional<Node::If> Parser::parse_if_stmt()
{
  RollbackGuard guard(m_index);
  auto tok = peek();
  if (!tok.has_value() || tok != TokenTypes::Keyword::IF)
    return {};

  consume();
  // Mandatory '(' expr ')'
  consume();
  auto cond = parse_expr();
  if (!cond.has_value()) return {};
  consume();

  // Mandatory '{' scope '}' (for now)
  auto scope = parse_scope();
  if (!scope.has_value()) return {};

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
        return {};
      }
      auto true_cond = std::make_shared<Node::Int>(Node::Int(1, tok1->loc));
      auto else_node = Node::If(true_cond, else_scope.value(), {}, else_scope->loc);
      auto res = Node::If(cond.value(), scope.value(), else_node, scope->loc);

      guard.commit();
      return res;
    }
    else
    {
      guard.commit();
      return Node::If(cond.value(), scope.value(), elif.value(), scope->loc);
    }
  }
  guard.commit();
  return Node::If(cond.value(), scope.value(), {}, scope->loc);
}

std::optional<Node::Import> Parser::parse_import()
{
  if (!peek().has_value() || peek().value() != TokenTypes::Keyword::IMPORT) return {};

  Node::Import import(consume().loc);
  while (peek().has_value() && peek().value() == TokenTypes::Literal::IDENT)
  {
    if (
      peek().has_value() && peek().value() == TokenTypes::Symbol::COL &&
      peek(1).has_value() && peek(1).value() == TokenTypes::Symbol::COL
      )
    {
      import.mod.emplace_back(consume().value.value(), SrcLoc({}, 0, 0, {}));
      consume();
      consume();
    }
    else if (peek().has_value() && peek().value() == TokenTypes::Symbol::SEMICOL)
    {
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

  Node::Scope scope(consume().loc);
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

    params.emplace_back(*type, std::move(name_str), SrcLoc({}, 0, 0, {}));
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
  if (!idt)
  {
    Diagnostic::error(peek()->loc, "Expected name after 'struct' keyword, found: {}", *peek());
    sync();
    return {};
  }
  Node::Struct struct_node(idt.value(), idt->loc);
  auto type = std::make_shared<Type>(Type::Struct(struct_node.id.name));
  auto& stype = std::get<Type::Struct>(type->type);
  m_tracker.push_type(struct_node.id.name, type);
  if (!peek() || peek() != TokenTypes::Symbol::LCUR)
  {
    Diagnostic::error(peek()->loc, "Expected struct scope, found: {}", *peek());
    sync();
    return {};
  }
  consume(); // {
  while (peek() && peek().value() != TokenTypes::Symbol::RCUR)
  {
    if (auto tp = parse_type())
    {
      if (!peek() || peek() != TokenTypes::Literal::IDENT)
      {
        Diagnostic::error(peek()->loc, "Expected member, found: {}", *peek());
        sync();
        return {};
      }
      auto tok = consume();
      auto mem = Node::Ident(tok.value.value(), tok.loc);
      struct_node.members.emplace_back(tp.value(), mem);
      
      if (!peek() || peek() != TokenTypes::Symbol::SEMICOL)
      {
        Diagnostic::error(peek()->loc, "Expected ';', found: {}", *peek());
        sync();
        return {};
      }
      consume(); // ;
    }
    else
    {
      Diagnostic::error(peek()->loc, "Expected something in decl scope, found: {}", *peek());
      sync();
      return {};
    }
  }
  if (!peek())
  {
    Diagnostic::error(m_tokens.back().loc, "Expected '}' found nothing");
    return {};
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
  if (!idt)
  {
    Diagnostic::error(peek()->loc, "Expected name after 'union' keyword, found: {}", *peek());
    sync();
    return {};
  }
  Node::Union union_node(idt.value(), idt->loc);
  auto type = std::make_shared<Type>(Type::Union(union_node.id.name));
  m_tracker.push_type(union_node.id.name, type);
  if (!peek() || peek() != TokenTypes::Symbol::LCUR)
  {
    Diagnostic::error(peek()->loc, "Expected union scope, found: {}", *peek());
    sync();
    return {};
  }
  consume(); // {
  while (peek() && peek().value() != TokenTypes::Symbol::RCUR)
  {
    if (auto tp = parse_type())
    {
      if (!peek() || peek() != TokenTypes::Literal::IDENT)
      {
        Diagnostic::error(peek()->loc, "Expected member, found: {}", *peek());
        sync();
        return {};
      }
      auto tok = consume();
      auto mem = Node::Ident(tok.value.value(), tok.loc);
      union_node.members.emplace_back(tp.value(), mem);
      if (!peek() || peek() != TokenTypes::Symbol::SEMICOL)
      {
        Diagnostic::error(peek()->loc, "Expected ';', found: {}", *peek());
        sync();
        return {};
      }
      consume(); // ;
    }
    else
    {
      Diagnostic::error(peek()->loc, "Expected something in decl scope, found: {}", *peek());
      sync();
      return {};
    }
  }
  if (!peek())
  {
    Diagnostic::error(m_tokens.back().loc, "Expected '}' found nothing");
    return {};
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
  if (!idt)
  {
    Diagnostic::error(peek()->loc, "Expected name after 'namespace' keyword, found: {}", *peek());
    sync();
    return {};
  }

  Node::Namespace ns_node(idt.value(), idt->loc);
  if (auto scp = parse_scope()) ns_node.scp = scp.value();
  else
  {
    Diagnostic::error(peek()->loc, "Expected namespace scope, found: {}", *peek());
    sync();
    return {};
  }

  return ns_node;
}

std::optional<Node::Enum> Parser::parse_enum()
{
  if (!peek() || peek().value() != TokenTypes::Keyword::ENUM) return {};
  consume();

  auto tok = consume();
  Node::Enum enm(tok.value.value(), tok.loc);
  std::shared_ptr<Node::Expr> prev = std::make_shared<Node::Int>(-1, SrcLoc({}, 0, 0, {}));
  if (!peek() || peek() != TokenTypes::Symbol::LCUR)
  {
    Diagnostic::error(peek()->loc, "Expected enum scope, found: {}", *peek());
    sync();
    return {};
  }
  consume(); // {
  while (peek() && peek().value() != TokenTypes::Symbol::RCUR)
  {
    auto en = consume();
    if (en != TokenTypes::Literal::IDENT)
    {
      Diagnostic::error(peek()->loc, "Expected enumerator, found: {}", *peek());
      sync();
      return {};
    }

    if (peek() && peek() == TokenTypes::Symbol::AEQ)
    {
      consume();
      if (auto expr = parse_expr())
      {
        prev = expr.value();
        auto asgn = Node::Asgn(
          Node::TypeRef(std::make_shared<Type>(Type::Base::I32), SrcLoc({}, 0, 0, {})),
          en.value.value(),
          expr.value(),
          en.loc
        );
        enm.enums.push_back(asgn);
      }
      else
      {
        Diagnostic::error(peek()->loc, "Expected expression, found: {}", *peek());
        sync();
        return {};
      }
    }
    else
    {
      prev = std::make_shared<Node::BinExpr>(
        prev,
        BinOp::ADD,
        std::make_shared<Node::Int>(1, SrcLoc({}, 0, 0, {})),
        SrcLoc({}, 0, 0, {})
      );
      auto asgn = Node::Asgn(
        Node::TypeRef(std::make_shared<Type>(Type::Base::I32), SrcLoc({}, 0, 0, {})),
        en.value.value(),
        prev,
        en.loc
      );
      enm.enums.push_back(asgn);
    }
    if (peek() && peek() == TokenTypes::Symbol::COM) consume(); // ,
    else
    {
      if (!peek() || peek() != TokenTypes::Symbol::RCUR)
      {
        Diagnostic::error(peek()->loc, "Expected '}', found: {}", *peek());
        sync();
        return {};
      }
      break;
    }
  }
  if (!peek())
  {
    Diagnostic::error(m_tokens.back().loc, "Expected '}' found nothing");
    return {};
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
  if (!peek() || peek() != TokenTypes::Literal::IDENT)
    return {};

  auto tok = consume();
  std::string name = tok.value.value();

  auto ident = Node::Ident(name, tok.loc);
  std::vector<Node::Param> params;
  auto ret_t = Node::TypeRef(std::make_shared<Type>(Type::Base::VOID), SrcLoc({}, 0, 0, {}));
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

  Node::Func func(std::move(ident), ret_t, ident.loc);
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

  auto fr = consume();
  Node::For loop(fr.loc);

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

  auto tok = consume();
  Node::Ret ret(parse_expr(), tok.loc);
  consume(); // ;
  return ret;
}

void Parser::parse_arr(std::variant<Node::TypeDef, Node::TypeRef>& type)
{
  auto tp = std::visit(
    Utils::overloaded
    {
      [](Node::TypeDef& td) -> std::shared_ptr<Type> { return td.res_t; },
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
    {
      Diagnostic::error(peek()->loc, "Expected expression in [...], found: {}", *peek());
      sync();
      return;
    }

    if (!peek() || peek() != TokenTypes::Symbol::RBRA)
    {
      Diagnostic::error(peek()->loc, "Expected ']', found: {}", *peek());
      sync();
      return;
    }
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
        return Node::Asgn(t.value(), ident.value.value(), expr.value(), ident.loc);
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
      return Node::Asgn(t.value(), ident.value.value(), ident.loc);
    }
  }
  return {};
}

std::optional<Node::Continue> Parser::parse_continue_stmt()
{
  if (peek() && peek() == TokenTypes::Keyword::CONTINUE)
  {
    auto tok = consume();
    if (!peek() || peek() != TokenTypes::Symbol::SEMICOL)
    {
      Diagnostic::error(peek()->loc, "Expected ';' after 'continue', found: {}", *peek());
      sync();
      return {};
    }
    consume();
    return Node::Continue(tok.loc);
  }
  return {};
}

std::optional<Node::Break> Parser::parse_break_stmt()
{
  if (peek() && peek() == TokenTypes::Keyword::BREAK)
  {
    auto tok = consume();
    if (!peek() || peek() != TokenTypes::Symbol::SEMICOL)
    {
      Diagnostic::error(peek()->loc, "Expected ';' after 'break', found: {}", *peek());
      sync();
      return {};
    }
    consume();
    return Node::Break(tok.loc);
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
  else if (auto scope_stmt = parse_scope())
  {
    return std::make_shared<Node::Scope>(scope_stmt.value());
  }
  else if (auto imp_stmt = parse_import())
  {
    return std::make_shared<Node::Import>(imp_stmt.value());
  }
  return {};
}

void Parser::sync()
{
  while (peek())
  {
    if (peek() == TokenTypes::Symbol::SEMICOL)
    {
      consume();
      break;
    }

    if (peek() == TokenTypes::Symbol::LCUR) return;
    if (peek() == TokenTypes::Keyword::IF) return;
    if (peek() == TokenTypes::Keyword::FN) return;
    if (peek() == TokenTypes::Keyword::STRUCT) return;
    if (peek() == TokenTypes::Keyword::UNION) return;
    if (peek() == TokenTypes::Keyword::ENUM) return;
    if (peek() == TokenTypes::Keyword::NAMEPSACE) return;
  }
}

[[nodiscard]] std::optional<Token> Parser::peek(const int offset)
{
  if (m_index + offset < m_tokens.size())
    return m_tokens[m_index + offset];
  return {};
}

Token Parser::consume() { return m_tokens[m_index++]; }

