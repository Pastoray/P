#include "sema.hpp"
#include "parser.hpp"
#include "utils.hpp"

#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <variant>


uint32_t Sema::Symbol::nid = 0;
Sema::Sema(const std::vector<Node::Node>& nodes)
    : m_nodes(nodes)
{
  auto global_scope = std::make_unique<Scope>();
  m_scope_stack.push_back(std::move(global_scope));
}

Sema::~Sema()
{
  while (!m_scope_stack.empty())
    pop_scope();
}

/*
void Sema::Visitor::visit(const Node::Func& func) { m_sema.analyze_func(func); }

void Sema::Visitor::visit(const Node::Struct& strct) { m_sema.analyze_struct(strct); }

void Sema::Visitor::visit(const Node::UnExpr& un_expr) { m_sema.analyze_unexpr(un_expr); }

void Sema::Visitor::visit(const Node::BinExpr& bin_expr) { m_sema.analyze_bin_expr(bin_expr); }

void Sema::Visitor::visit(const Node::Ident& ident) { m_sema.analyze_ident(ident); }

void Sema::Visitor::visit(const Node::Int& int_) { m_sema.analyze_int(int_); }

void Sema::Visitor::visit(const Node::Call& call) { m_sema.analyze_call(call); }

void Sema::Visitor::visit(const Node::Asgn& asgn) { m_sema.analyze_asgn(asgn); }

void Sema::Visitor::visit(const Node::If& if_) { m_sema.analyze_if(if_); }

void Sema::Visitor::visit(const Node::For& for_) { m_sema.analyze_for(for_); }

void Sema::Visitor::visit(const Node::Ret& ret) { m_sema.analyze_ret(ret); }

void Sema::Visitor::visit(const Node::ExprStmt& expr_stmt) { m_sema.analyze_expr_stmt(expr_stmt); }
*/

Sema::Analysis Sema::analyze()
{
  for (auto& node : m_nodes)
  {
    if (auto stmt = std::get_if<std::shared_ptr<Node::Stmt>>(&node))
      analyze_stmt(*stmt);
    else if (auto expr = std::get_if<std::shared_ptr<Node::Expr>>(&node))
      analyze_expr(*expr);
    else if (auto decl = std::get_if<std::shared_ptr<Node::Decl>>(&node))
      analyze_decl(*decl);
    else
      assert(false);
  }
  return g_anl;
}

void Sema::analyze_stmt(const std::shared_ptr<Node::Stmt>& stmt)
{
  if (auto asgn = std::dynamic_pointer_cast<Node::Asgn>(stmt))
    analyze_asgn(*asgn);
  else if (auto if_ = std::dynamic_pointer_cast<Node::If>(stmt))
    analyze_if(*if_);
  else if (auto for_ = std::dynamic_pointer_cast<Node::For>(stmt))
    analyze_for(*for_);
  else if (auto ret = std::dynamic_pointer_cast<Node::Ret>(stmt))
    analyze_ret(*ret);
  else if (auto expr_stmt = std::dynamic_pointer_cast<Node::ExprStmt>(stmt))
    analyze_expr_stmt(*expr_stmt);
  else if (auto cont = std::dynamic_pointer_cast<Node::Continue>(stmt))
    return;
  else if (auto brk = std::dynamic_pointer_cast<Node::Break>(stmt))
    return;
  else
    assert(false);
}

Sema::ExprInfo Sema::analyze_bin_expr(const Node::BinExpr& bin_expr)
{
  auto lhs_expri = analyze_expr(bin_expr.lhs);
  auto rhs_expri = analyze_expr(bin_expr.rhs);
  // assert(lhs_expri.type == rhs_expri.type && "Undefined operations between different types");

  return ExprInfo(lhs_expri.type, ValCat::RVALUE);
}

Sema::ExprInfo Sema::analyze_un_expr(const Node::UnExpr& un_expr)
{
  auto expri = analyze_expr(un_expr.expr);
  if (un_expr.op == UnOp::DEREF)
    return ExprInfo(expri.type, ValCat::LVALUE);
  else if (un_expr.op == UnOp::ADDR_OF)
  {
    if (expri.val_cat == ValCat::RVALUE)
      Utils::panic("Invalid use of addrof on rvalue");
    else
      return ExprInfo(expri.type, ValCat::RVALUE);
  }
  else
    return ExprInfo(expri.type, ValCat::RVALUE);
}

Sema::ExprInfo Sema::analyze_call(const Node::Call& call)
{
  bool found = false;
  for (int32_t i = (int32_t)m_scope_stack.size() - 1; i >= 0; i--)
  {
    found = (
      m_scope_stack[i]->m_sym_table.find(call.callable.name) != m_scope_stack[i]->m_sym_table.end() &&
      m_scope_stack[i]->m_sym_table.at(call.callable.name).is_fn()
    );
    if (found)
    {
      // g_anl.sym_table.try_emplace(call.callable.id, m_scope_stack[i]->m_sym_table.at(call.callable.name));
      for (auto& arg : call.args)
        analyze_expr(arg);
      return Sema::ExprInfo(m_scope_stack[i]->m_sym_table.at(call.callable.name).type, ValCat::RVALUE);
    }
  }
  Utils::panic("Function '" + call.callable.name + "' does not exist");
}

Sema::ExprInfo Sema::analyze_expr(const std::shared_ptr<Node::Expr>& expr)
{
  if (auto bin_expr = std::dynamic_pointer_cast<Node::BinExpr>(expr))
    return analyze_bin_expr(*bin_expr);
  else if (auto un_expr = std::dynamic_pointer_cast<Node::UnExpr>(expr))
    return analyze_un_expr(*un_expr);
  else if (auto lit = std::dynamic_pointer_cast<Node::Lit>(expr))
    return analyze_lit(lit);
  else if (auto call = std::dynamic_pointer_cast<Node::Call>(expr))
    return analyze_call(*call);
  Utils::panic("Unexpected analyze expression");
}

Sema::ExprInfo Sema::analyze_ident(const Node::Ident& ident)
{
  for (int32_t i = (int32_t)m_scope_stack.size() - 1; i >= 0; i--)
    if (m_scope_stack[i]->m_sym_table.find(ident.name) != m_scope_stack[i]->m_sym_table.end())
    {
      g_anl.sym_table.try_emplace(ident.id, m_scope_stack[i]->m_sym_table.at(ident.name));
      return ExprInfo(m_scope_stack[i]->m_sym_table.at(ident.name).type, ValCat::LVALUE);
    }

  Utils::panic("Identifier '" + ident.name + "' does not exist");
}

Sema::ExprInfo Sema::analyze_int(const Node::Int& int_)
{
  return ExprInfo(Type(Type::Base::I32), ValCat::RVALUE);
}

Sema::ExprInfo Sema::analyze_lit(const std::shared_ptr<Node::Lit>& lit)
{
  if (auto int_ = std::dynamic_pointer_cast<Node::Int>(lit))
    return analyze_int(*int_);
  else if (auto ident = std::dynamic_pointer_cast<Node::Ident>(lit))
    return analyze_ident(*ident);
  Utils::panic("Unexpected analyze literal");
}

void Sema::analyze_decl(const std::shared_ptr<Node::Decl>& decl)
{
  if (auto strct = std::dynamic_pointer_cast<Node::Struct>(decl))
    analyze_struct(*strct);

  else if (auto func = std::dynamic_pointer_cast<Node::Func>(decl))
    analyze_func(*func);
}

void Sema::analyze_asgn(const Node::Asgn& asgn)
{
  analyze_type_usage(asgn.type);
  if (get_curr_scope()->m_sym_table.find(asgn.id.name) != get_curr_scope()->m_sym_table.end())
    Utils::panic("Variable redeclaration detected");

  auto t = *asgn.type;
  while (auto* arr = std::get_if<Type::Arr>(&t.type))
  {
    auto dim = std::static_pointer_cast<Node::Expr>(arr->size);
    analyze_expr(dim);
    t = *arr->of;
  }

  m_scope_stack.back()->m_sym_table.emplace(asgn.id.name, Sema::Symbol(t, Sema::Symbol::VarExt(get_curr_scope()->prev == nullptr)));
  g_anl.sym_table.try_emplace(asgn.id.id, m_scope_stack.back()->m_sym_table.at(asgn.id.name));

  if (asgn.val)
  {
    if (auto expr = std::get_if<std::shared_ptr<Node::Expr>>(&asgn.val.value()))
      analyze_expr(*expr);
    else if (auto decl = std::get_if<std::shared_ptr<Node::Decl>>(&asgn.val.value()))
      analyze_decl(*decl);
    else
      assert(false);
  }
}

void Sema::analyze_if(const Node::If& if_)
{
  analyze_expr(if_.cond);

  auto sc = std::make_unique<Scope>();
  sc->prev = m_scope_stack.back().get();
  m_scope_stack.push_back(std::move(sc));
  analyze_scope(*if_.scope);
  pop_scope();

  if (if_.elif->has_value()) analyze_if(**if_.elif);
}

void Sema::analyze_for(const Node::For& for_)
{
  auto sc = std::make_unique<Scope>();
  sc->prev = m_scope_stack.back().get();
  m_scope_stack.push_back(std::move(sc));

  if (for_.init) analyze_asgn(**for_.init);
  if (for_.cond) analyze_expr(*for_.cond);
  if (for_.adv) analyze_expr(*for_.adv);
  analyze_scope(*for_.scope);

  pop_scope();
}

void Sema::analyze_ret(const Node::Ret& ret)
{
  if (ret.ret_val) analyze_expr(*ret.ret_val);
}

Sema::ExprInfo Sema::analyze_expr_stmt(const Node::ExprStmt& expr_stmt)
{
  return analyze_expr(expr_stmt.expr);
}

void Sema::analyze_type_usage(const std::shared_ptr<Type>& type)
{
  if (!is_valid_type(*type))
    Utils::panic("Invalid type encountered...");
}

void Sema::analyze_func(const Node::Func& func)
{
  if (
    get_curr_scope()->m_sym_table.find(func.id.name) != get_curr_scope()->m_sym_table.end() &&
    get_curr_scope()->m_sym_table.at(func.id.name).is_fn()
  )
    Utils::panic("Function '" + func.id.name + "' already exist");

  auto fn_ext = Sema::Symbol::FnExt(func.id.name, func.params);
  get_curr_scope()->m_sym_table.try_emplace(func.id.name, Sema::Symbol(func.ret_type, fn_ext));
  g_anl.sym_table.try_emplace(func.id.id, get_curr_scope()->m_sym_table.at(func.id.name));

  /*
  for (auto& param : func.params)
  {
    get_curr_scope()->m_sym_table.try_emplace(func.id.name, Sema::Symbol(func.ret_type, fn_ext));
    g_anl.sym_table.try_emplace(func.id.id, get_curr_scope()->m_sym_table.at(func.id.name));
  }
  */

  // Func is now nested twice as a workaround for not having potential redifinition errors
  if (func.scope.has_value())
  {
    auto sc = std::make_unique<Scope>();
    sc->prev = m_scope_stack.back().get();
    m_scope_stack.push_back(std::move(sc));

    for (auto& param : func.params)
    {
      get_curr_scope()->m_sym_table.try_emplace(param.name, Sema::Symbol(param.type, Symbol::VarExt()));
      g_anl.sym_table.try_emplace(param.id, get_curr_scope()->m_sym_table.at(param.name));
    }

    auto sc2 = std::make_unique<Scope>();
    sc2->prev = m_scope_stack.back().get();
    m_scope_stack.push_back(std::move(sc2));
    analyze_scope(func.scope.value());
    pop_scope();
    pop_scope();
  }
}

void Sema::analyze_scope(const Node::Scope& scope)
{
  for (auto& node : scope.terms)
  {
    if (auto stmt = std::get_if<std::shared_ptr<Node::Stmt>>(&node))
      analyze_stmt(*stmt);
    else if (auto expr = std::get_if<std::shared_ptr<Node::Expr>>(&node))
      analyze_expr(*expr);
    else if (auto decl = std::get_if<std::shared_ptr<Node::Decl>>(&node))
      analyze_decl(*decl);
  }
}

void Sema::analyze_struct(const Node::Struct& strct)
{
  if (
    get_curr_scope()->m_sym_table.find(strct.id.name) != get_curr_scope()->m_sym_table.end() &&
    get_curr_scope()->m_sym_table.at(strct.id.name).is_st()
  )
    Utils::panic("Struct redifinition for struct: '" + strct.id.name + "'");

  for (auto& [type, field] : strct.members)
    if (!is_valid_type(type))
      Utils::panic("Unknown type '" + Type::to_string(type) + "' in struct definition");

  auto ct = Type::Custom(strct.id.name);
  auto strct_t = Type(ct);
  auto strct_ext = Sema::Symbol::StcExt(0);

  get_curr_scope()->m_sym_table.try_emplace(strct.id.name, Sema::Symbol(strct_t, strct_ext));
  g_anl.sym_table.try_emplace(strct.id.id, get_curr_scope()->m_sym_table.at(strct.id.name));
  get_curr_scope()->m_types_table.insert(ct);
}

Sema::Scope* Sema::get_curr_scope()
{
  return m_scope_stack.back().get();
}

void Sema::pop_scope()
{
  assert(!m_scope_stack.empty());
  // get_curr_scope()->prev->next = nullptr;
  m_scope_stack.pop_back();
}

Sema::Module* Sema::load_module()
{
  return nullptr;
}

bool Sema::is_valid_type(const Type& type)
{
  return std::visit(
    Utils::overloaded
    {
      [&](const Type::Base& b) { return true; },
      [&](const Type::Ptr& p) { return is_valid_type(*p.to); },
      [&](const Type::Arr& a) { return is_valid_type(*a.of); },
      [&](const Type::Custom& c) { return get_curr_scope()->m_types_table.find(c) != get_curr_scope()->m_types_table.end(); },
    }, type.type
  );
  return true;
}

