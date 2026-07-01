#include "sema.hpp"
#include "parser.hpp"
#include "type.hpp"
#include "utils.hpp"

#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <variant>


uint32_t Sema::Symbol::nid = 0;
Sema::Sema(const std::vector<Node::Node>& nodes) : m_nodes(nodes)
{
  m_scope_stack.push_back(m_scope_arena.size());
  m_scope_arena.push_back(std::make_unique<Scope>());
}

Sema::~Sema()
{
  while (!m_scope_stack.empty())
    pop_scope();
}

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
  else if (auto td = std::dynamic_pointer_cast<Node::TypeDef>(stmt))
    analyze_type_def(*td);
  else if (auto cont = std::dynamic_pointer_cast<Node::Continue>(stmt))
    return;
  else if (auto brk = std::dynamic_pointer_cast<Node::Break>(stmt))
    return;
  else
    assert(false);
}

Sema::ExprInfo Sema::analyze_bin_expr(Node::BinExpr& bin_expr)
{
  auto lhs_expri = analyze_expr(bin_expr.lhs);
  if (bin_expr.op == BinOp::DOT)
  {
    auto rhs = std::dynamic_pointer_cast<Node::Ident>(bin_expr.rhs);
    assert(rhs != nullptr);

    if (auto* stc_t = std::get_if<Type::Struct>(&lhs_expri.type.type))
    {
      size_t off = stc_t->body->offsets.at(rhs->name);
      bin_expr.rhs = std::make_shared<Node::Int>(off);
      return ExprInfo(*stc_t->body->types.at(rhs->name), ValCat::LVALUE);
    }
    else if (auto* un_t = std::get_if<Type::Union>(&lhs_expri.type.type))
    {
      size_t off = un_t->body->offsets.at(rhs->name);
      bin_expr.rhs = std::make_shared<Node::Int>(off);
      return ExprInfo(*un_t->body->types.at(rhs->name), ValCat::LVALUE);
    }
    else if (auto* en_t = std::get_if<Type::Enum>(&lhs_expri.type.type))
    {
      auto expr = std::static_pointer_cast<Node::Expr>(en_t->enums.at(rhs->name));
      auto bexpr = Node::BinExpr(expr, BinOp::ADD, std::make_shared<Node::Int>(0));
      bin_expr = bexpr;
      return ExprInfo(Type(Type::Base::I32), ValCat::RVALUE);
    }
    else assert(false);
  }
  else if (bin_expr.op == BinOp::ARROW)
  {
    auto rhs = std::dynamic_pointer_cast<Node::Ident>(bin_expr.rhs);
    assert(rhs != nullptr);

    auto* ptr_t = std::get_if<Type::Ptr>(&lhs_expri.type.type);
    assert(ptr_t != nullptr);

    if (auto* stc_t = std::get_if<Type::Struct>(&ptr_t->to->type))
    {
      size_t off = stc_t->body->offsets.at(rhs->name);
      bin_expr.rhs = std::make_shared<Node::Int>(off);
      return ExprInfo(*stc_t->body->types.at(rhs->name), ValCat::LVALUE);
    }
    else if (auto* un_t = std::get_if<Type::Union>(&ptr_t->to->type))
    {
      size_t off = un_t->body->offsets.at(rhs->name);
      bin_expr.rhs = std::make_shared<Node::Int>(off);
      return ExprInfo(*un_t->body->types.at(rhs->name), ValCat::LVALUE);
    }
    else assert(false);
  }
  auto rhs_expri = analyze_expr(bin_expr.rhs);

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
  auto info = analyze_expr(call.callable);
  assert(info.type.is_func_t());

  for (auto& arg : call.args)
    analyze_expr(arg);
  return Sema::ExprInfo(info.type, ValCat::RVALUE);
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

Sema::ExprInfo Sema::analyze_path(const Node::Path& path)
{
  int scpid = -1;
  for (int32_t i = (int32_t)m_scope_stack.size() - 1; i >= 0; i--)
    if (
      auto it = m_scope_arena[m_scope_stack[i]]->m_sym_table.find(path.path[0].name);
      it != m_scope_arena[m_scope_stack[i]]->m_sym_table.end() && it->second.is_ns()
    )
    {
      auto sym = m_scope_arena[m_scope_stack[i]]->m_sym_table.at(path.path[i].name);
      auto ext = std::get<Sema::Symbol::NsExt>(sym.ext);
      scpid = ext.scp_id;
      break;
    }
  
  assert(scpid != -1);
  for (size_t i = 1; i < path.path.size(); i++)
  {
    if (
      auto it = m_scope_arena[scpid]->m_sym_table.find(path.path[i].name);
      it != m_scope_arena[scpid]->m_sym_table.end()
    )
    {
      auto sym = m_scope_arena[scpid]->m_sym_table.at(path.path[i].name);
      if (sym.is_ns())
      {
        assert(i != path.path.size() - 1);
        auto ext = std::get<Sema::Symbol::NsExt>(sym.ext);
        scpid = ext.scp_id;
      }
      else
      {
        assert(i == path.path.size() - 1);
        g_anl.sym_table.try_emplace(path.path[i].id, m_scope_arena[scpid]->m_sym_table.at(path.path[i].name));
        return ExprInfo(sym.type, ValCat::LVALUE);
      }
    }
    else assert(false);
  }
  assert(false);
}

Sema::ExprInfo Sema::analyze_ident(const Node::Ident& ident)
{
  for (int32_t i = (int32_t)m_scope_stack.size() - 1; i >= 0; i--)
    if (m_scope_arena[m_scope_stack[i]]->m_sym_table.find(ident.name) != m_scope_arena[m_scope_stack[i]]->m_sym_table.end())
    {
      g_anl.sym_table.try_emplace(ident.id, m_scope_arena[m_scope_stack[i]]->m_sym_table.at(ident.name));
      return ExprInfo(m_scope_arena[m_scope_stack[i]]->m_sym_table.at(ident.name).type, ValCat::LVALUE);
    }

  Utils::panic("Identifier '" + ident.name + "' does not exist");
}

Sema::ExprInfo Sema::analyze_int(const Node::Int& int_)
{
  return ExprInfo(Type(Type::Base::I32), ValCat::RVALUE);
}

Sema::ExprInfo Sema::analyze_double(const Node::Double& double_)
{
  return ExprInfo(Type(Type::Base::F64), ValCat::RVALUE);
}

Sema::ExprInfo Sema::analyze_string(const Node::String& str)
{
  return ExprInfo(Type(Type::Ptr(std::make_shared<Type>(Type::Base::CHAR))), ValCat::RVALUE);
}

Sema::ExprInfo Sema::analyze_char(const Node::Char& c)
{
  return ExprInfo(Type(Type::Base::CHAR), ValCat::RVALUE);
}

Sema::ExprInfo Sema::analyze_lit(const std::shared_ptr<Node::Lit>& lit)
{
  if (auto int_ = std::dynamic_pointer_cast<Node::Int>(lit))
    return analyze_int(*int_);
  if (auto double_ = std::dynamic_pointer_cast<Node::Double>(lit))
    return analyze_double(*double_);
  if (auto string = std::dynamic_pointer_cast<Node::String>(lit))
    return analyze_string(*string);
  if (auto c = std::dynamic_pointer_cast<Node::Char>(lit))
    return analyze_char(*c);
  if (auto path = std::dynamic_pointer_cast<Node::Path>(lit))
    return analyze_path(*path);
  if (auto ident = std::dynamic_pointer_cast<Node::Ident>(lit))
    return analyze_ident(*ident);
  Utils::panic("Unexpected analyze literal");
}

void Sema::analyze_decl(const std::shared_ptr<Node::Decl>& decl)
{
  if (auto func = std::dynamic_pointer_cast<Node::Func>(decl))
    analyze_func(*func);

  else if (auto ns = std::dynamic_pointer_cast<Node::Namespace>(decl))
    analyze_namespace(*ns);
}

void Sema::analyze_asgn(const Node::Asgn& asgn)
{
  std::visit(
    Utils::overloaded
    {
      [&](const Node::TypeDef& def) { analyze_type_def(def); },
      [&](const Node::TypeRef& ref) { analyze_type_ref(ref); }
    }, asgn.type
  );
  if (get_curr_scope()->m_sym_table.find(asgn.id.name) != get_curr_scope()->m_sym_table.end())
    Utils::panic("Variable redeclaration detected");

  auto t = std::visit(
    Utils::overloaded
    {
      [&](const Node::TypeDef& def) -> Type { return *def.res_t; },
      [&](const Node::TypeRef& ref) -> Type { return *ref.res_t; }
    }, asgn.type
  );
  while (auto* arr = std::get_if<Type::Arr>(&t.type))
  {
    auto dim = std::static_pointer_cast<Node::Expr>(arr->size);
    analyze_expr(dim);
    t = *arr->of;
  }

  {
    std::string name;
    if (get_curr_scope()->prev == nullptr && m_mang_pref.empty())
      name = asgn.id.name;
    else
    {
      std::string mname = "_ZN";
      for (auto& str : m_mang_pref)
        mname += std::to_string(str.size()) + str;
      mname += std::to_string(asgn.id.name.size()) + asgn.id.name;
      name = mname;
    }

    Sema::Symbol::VarExt sym(name, get_curr_scope()->prev == nullptr);
    m_scope_arena[m_scope_stack.back()]->m_sym_table.emplace(
      asgn.id.name, Sema::Symbol(t, sym)
    );
    g_anl.sym_table.try_emplace(asgn.id.id, m_scope_arena[m_scope_stack.back()]->m_sym_table.at(asgn.id.name));
  }

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

  m_scope_arena.push_back(std::make_unique<Scope>());
  auto& sc = m_scope_arena.back();
  sc->prev = m_scope_arena[m_scope_stack.back()].get();
  m_scope_stack.push_back(m_scope_arena.size() - 1);

  analyze_scope(*if_.scope);
  pop_scope();

  if (if_.elif->has_value()) analyze_if(**if_.elif);
}

void Sema::analyze_for(const Node::For& for_)
{
  m_scope_arena.push_back(std::make_unique<Scope>());
  auto& sc = m_scope_arena.back();
  sc->prev = m_scope_arena[m_scope_stack.back()].get();
  m_scope_stack.push_back(m_scope_arena.size() - 1);

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

void Sema::analyze_type_def(const Node::TypeDef& def)
{

  auto analyze_type = [&](const std::variant<Node::TypeDef, Node::TypeRef>& var)
  {
    return std::visit(
      Utils::overloaded
      {
        [&](const Node::TypeDef& def) { analyze_type_def(def); },
        [&](const Node::TypeRef& ref) { analyze_type_ref(ref); }
      }, var
    );
  };

  std::visit(
    Utils::overloaded
    {
      [&](const std::shared_ptr<Node::Struct>& stc)
      {
        auto stc_res_t = std::get_if<Type::Struct>(&def.res_t->type);
        assert(stc_res_t);
        for (auto& [type, member] : stc->members)
        {
          auto tp = Node::get_res_t(type);
          analyze_type(type);
          assert(tp->is_complete());
          stc_res_t->insert_mem(tp, member.name);
        }
        stc_res_t->finish();
        
        auto strct_ext = Sema::Symbol::StcExt();
        get_curr_scope()->m_sym_table.try_emplace(stc->id.name, Sema::Symbol(Type(*stc_res_t), strct_ext));
        g_anl.sym_table.try_emplace(stc->id.id, get_curr_scope()->m_sym_table.at(stc->id.name));
        get_curr_scope()->m_types_table.insert(stc->id.name);
      },
      [&](const std::shared_ptr<Node::Union>& un)
      {
        auto un_res_t = std::get_if<Type::Union>(&def.res_t->type);
        assert(un_res_t);
        for (auto& [type, member] : un->members)
        {
          auto tp = Node::get_res_t(type);
          analyze_type(type);
          assert(tp->is_complete());
          un_res_t->insert_mem(tp, member.name);
        }
        un_res_t->finish();

        auto uni_ext = Sema::Symbol::UniExt();
        get_curr_scope()->m_sym_table.try_emplace(un->id.name, Sema::Symbol(Type(*un_res_t), uni_ext));
        g_anl.sym_table.try_emplace(un->id.id, get_curr_scope()->m_sym_table.at(un->id.name));
        get_curr_scope()->m_types_table.insert(un->id.name);
      },
      [&](const std::shared_ptr<Node::Enum>& en)
      {
        auto en_res_t = std::get_if<Type::Enum>(&def.res_t->type);
        assert(en_res_t);
        for (auto& asgn : en->enums)
        {
          assert(asgn.val.has_value() && std::holds_alternative<std::shared_ptr<Node::Expr>>(asgn.val.value()));
          auto v = std::get<std::shared_ptr<Node::Expr>>(asgn.val.value());
          en_res_t->enums.try_emplace(asgn.id.name, std::move(v));
          analyze_asgn(asgn);
        }
        en_res_t->state = Type::TypeState::RESOLVED;

        auto enu_ext = Sema::Symbol::EnuExt();
        get_curr_scope()->m_sym_table.try_emplace(en->id.name, Sema::Symbol(Type(*en_res_t), enu_ext));
        g_anl.sym_table.try_emplace(en->id.id, get_curr_scope()->m_sym_table.at(en->id.name));
        get_curr_scope()->m_types_table.insert(en->id.name);
      }
    }, def.type
  );
}

void Sema::analyze_type_ref(const Node::TypeRef& ref)
{ assert(ref.res_t->is_complete() && "Type reference not resolved"); }

void Sema::analyze_func(const Node::Func& func)
{
  if (
    get_curr_scope()->m_sym_table.find(func.id.name) != get_curr_scope()->m_sym_table.end() &&
    get_curr_scope()->m_sym_table.at(func.id.name).is_fn()
  )
    Utils::panic("Function '" + func.id.name + "' already exist");

  std::string name;
  if (get_curr_scope()->prev == nullptr)
    name = func.id.name;
  else
  {
    std::string mname = "_ZN";
    for (auto& str : m_mang_pref)
      mname += std::to_string(str.size()) + str;
    mname += std::to_string(func.id.name.size()) + func.id.name;
    name = mname;
  }
  auto fn_ext = Sema::Symbol::FnExt(name, func.params);
  std::vector<std::shared_ptr<Type>> param_types;
  for (auto& p : func.params)
    if (!p.type.res_t->is_complete()) assert(false);
  for (auto& p : func.params) param_types.push_back(p.type.res_t);
  assert(
    func.ret_type.res_t->is_complete() ||
    (func.ret_type.res_t->is_base_t() && std::get<Type::Base>(func.ret_type.res_t->type) == Type::Base::VOID)
  );
  auto tp = Type(Type::Func(param_types, func.ret_type.res_t));
  get_curr_scope()->m_sym_table.try_emplace(func.id.name, Sema::Symbol(tp, fn_ext));
  g_anl.sym_table.try_emplace(func.id.id, get_curr_scope()->m_sym_table.at(func.id.name));

  // Func is now nested twice as a workaround for not having potential redifinition errors
  if (func.scope.has_value())
  {
    m_scope_arena.push_back(std::make_unique<Scope>());
    auto& sc = m_scope_arena.back();
    sc->prev = m_scope_arena[m_scope_stack.back()].get();
    m_scope_stack.push_back(m_scope_arena.size() - 1);

    for (auto& param : func.params)
    {
      get_curr_scope()->m_sym_table.try_emplace(param.name, Sema::Symbol(*param.type.res_t, Symbol::VarExt()));
      g_anl.sym_table.try_emplace(param.id, get_curr_scope()->m_sym_table.at(param.name));
    }

    m_scope_arena.push_back(std::make_unique<Scope>());
    auto& sc2 = m_scope_arena.back();
    sc2->prev = m_scope_arena[m_scope_stack.back()].get();
    m_scope_stack.push_back(m_scope_arena.size() - 1);

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

void Sema::analyze_namespace(const Node::Namespace& ns)
{
  if (
    get_curr_scope()->m_sym_table.find(ns.id.name) != get_curr_scope()->m_sym_table.end() &&
    get_curr_scope()->m_sym_table.at(ns.id.name).is_ns()
  )
  {
    auto& v = get_curr_scope()->m_sym_table.at(ns.id.name);
    assert(v.is_ns());
    auto scpid = std::get<Sema::Symbol::NsExt>(v.ext).scp_id;

    m_scope_stack.push_back(scpid);
    m_mang_pref.push_back(std::to_string(ns.id.name.size()) + ns.id.name);
    analyze_scope(ns.scp);
    m_mang_pref.pop_back();
    m_scope_stack.pop_back();
  }
  else
  {
    uint32_t i = m_scope_arena.size();
    m_scope_arena.push_back(std::make_unique<Scope>());
    m_scope_stack.push_back(i);
    m_mang_pref.push_back(std::to_string(ns.id.name.size()) + ns.id.name);
    analyze_scope(ns.scp);
    m_mang_pref.pop_back();
    m_scope_stack.pop_back();

    get_curr_scope()->m_sym_table.try_emplace(ns.id.name, Sema::Symbol(Type(Type::Base::VOID), Sema::Symbol::NsExt(i)));
    g_anl.sym_table.try_emplace(ns.id.id, get_curr_scope()->m_sym_table.at(ns.id.name));
  }
}

Sema::Scope* Sema::get_curr_scope()
{
  return m_scope_arena[m_scope_stack.back()].get();
}

void Sema::push_scope()
{
  m_scope_stack.push_back(m_scope_arena.size());
  m_scope_arena.push_back(std::make_unique<Scope>());
}

void Sema::pop_scope()
{
  assert(!m_scope_stack.empty());
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
      [&](const Type::Struct& s)
      {
        auto cur = get_curr_scope();
        for (
          ;
          cur && cur->m_types_table.find(s.name) == cur->m_types_table.end();
          cur = cur->prev
        );
        return cur != nullptr;
      },
      [&](const Type::Union& u)
      {
        auto cur = get_curr_scope();
        for (
          ;
          cur && cur->m_types_table.find(u.name) == cur->m_types_table.end();
          cur = cur->prev
        );
        return cur != nullptr;
      },
      [&](const Type::Enum& e)
      {
        auto cur = get_curr_scope();
        for (
          ;
          cur && cur->m_types_table.find(e.name) == cur->m_types_table.end();
          cur = cur->prev
        );
        return cur != nullptr;
      },
      [&](const Type::Func& f)
      {
        bool ok = true;
        for (auto& p : f.params) ok &= is_valid_type(*p);
        ok &= is_valid_type(*f.ret);
        return ok;
      }
    }, type.type
  );
  return true;
}

