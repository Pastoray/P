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
  /*
  if (bin_expr.op == BinOp::DOT)
  {
    auto lhs = std::dynamic_pointer_cast<Node::Ident>(bin_expr.lhs);
    assert(lhs != nullptr);

    auto rhs = std::dynamic_pointer_cast<Node::Ident>(bin_expr.rhs);
    assert(rhs != nullptr);

    if (
      g_anl.sym_table.find(lhs->id) != g_anl.sym_table.end() &&
      std::get_if<Type::Struct>(&g_anl.sym_table.at(lhs->id).type.type)
    )
    {
      auto stc_t = std::get_if<Type::Struct>(&g_anl.sym_table.at(lhs->id).type.type);
      // assert(stc_t != nullptr);

      auto ret_t = stc_t->types.at(rhs->name);
      return Sema::ExprInfo(ret_t, ValCat::LVALUE);
    }

    assert(0);
  }
  */
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
  /*
  else if (bin_expr.op == BinOp::NSR)
  {
    auto lhs = std::dynamic_pointer_cast<Node::Ident>(bin_expr.lhs);
    if (lhs == nullptr)
    {
      std::cout << "ERROR" << std::endl;
      bin_expr.dump(0);
    }
    assert(lhs != nullptr);
    Sema::Symbol* ctx = nullptr;
    for (int i = m_scope_stack.size() - 1; i >= 0; i--)
    {
      auto& cs = m_scope_arena[m_scope_stack[i]];
      if (cs->m_sym_table.find(lhs->name) != cs->m_sym_table.end() && cs->m_sym_table.at(lhs->name).is_ns())
        ctx = &cs->m_sym_table.at(lhs->name);
    }

    assert(ctx && ctx->is_ns());
    auto ctx_ext = std::get<Sema::Symbol::NsExt>(ctx->ext);

    auto prev_scopes = m_scope_stack;
    m_scope_stack.clear();
    m_scope_stack.push_back(ctx_ext.scp_id);
    auto rhs_expri = analyze_expr(bin_expr.rhs);
    m_scope_stack.pop_back();
    m_scope_stack = prev_scopes;
    return rhs_expri;
  }
  */

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
  auto info = analyze_expr(call.callable);
  assert(info.type.is_func_t());

  for (auto& arg : call.args)
    analyze_expr(arg);
  return Sema::ExprInfo(info.type, ValCat::RVALUE);

  /*
  bool found = false;
  for (int32_t i = (int32_t)m_scope_stack.size() - 1; i >= 0; i--)
  {
    found = (
      m_scope_arena[m_scope_stack[i]]->m_sym_table.find(call.callable.name) !=
      m_scope_arena[m_scope_stack[i]]->m_sym_table.end() &&
      m_scope_arena[m_scope_stack[i]]->m_sym_table.at(call.callable.name).is_fn()
    );
    if (found)
    {
      assert(g_anl.sym_table.find(call.callable.id) == g_anl.sym_table.end());
      g_anl.sym_table.try_emplace(call.callable.id, m_scope_arena[m_scope_stack[i]]->m_sym_table.at(call.callable.name));
    }
    
  }
  */
  // Utils::panic("Function '" + call.callable.name + "' does not exist");
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
  if (auto strct = std::dynamic_pointer_cast<Node::Struct>(decl))
    analyze_struct(*strct);

  else if (auto un = std::dynamic_pointer_cast<Node::Union>(decl))
    analyze_union(*un);

  else if (auto func = std::dynamic_pointer_cast<Node::Func>(decl))
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
  // auto t = *asgn.type;
  while (auto* arr = std::get_if<Type::Arr>(&t.type))
  {
    auto dim = std::static_pointer_cast<Node::Expr>(arr->size);
    analyze_expr(dim);
    t = *arr->of;
  }

  /*
  if (auto* stc_t = std::get_if<Type::Struct>(&t.type))
  {
    bool ok = false;
    for (int i = (int)m_scope_stack.size() - 1; i >= 0; i--)
    {
      if (m_scope_stack[i]->m_sym_table.count(stc_t->name))
      {
        m_scope_stack.back()->m_sym_table.emplace(
          asgn.id.name,
          m_scope_stack[i]->m_sym_table.at(stc_t->name)
        );
        // populate_stc(asgn.id.name, stc_t);
        g_anl.sym_table.try_emplace(asgn.id.id, m_scope_stack.back()->m_sym_table.at(asgn.id.name));
        ok = true;
        break;
      }
    }
    assert(ok);
  }
  */
  // else
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
    /*
    m_scope_arena[m_scope_stack.back()]->m_sym_table.emplace(
      asgn.id.name, Sema::Symbol(t, Sema::Symbol::VarExt(get_curr_scope()->prev == nullptr))
    );
    g_anl.sym_table.try_emplace(asgn.id.id, m_scope_arena[m_scope_stack.back()]->m_sym_table.at(asgn.id.name));
    */
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
{
  assert(ref.res_t->is_complete() && "Type reference not resolved");
  /*
  auto analyze_type = [&](auto&& self, std::shared_ptr<Type>& type) -> void
  {
    std::visit(
      Utils::overloaded
      {
        [&](Type::Base& b) -> void {},
        [&](Type::Ptr& p) -> void { self(self, p.to); },
        [&](Type::Arr& a) -> void { self(self, a.of); },
        [&](Type::Struct& s) -> void
        {
          auto cur = get_curr_scope();
          for (
            ;
            cur && cur->m_types_table.find(s.name) == cur->m_types_table.end();
            cur = cur->prev
          );
          assert(cur != nullptr);
          auto sym = cur->m_sym_table.at(s.name);
          assert(sym.type.is_struct_t());
          s = std::get<Type::Struct>(sym.type.type);
        },
        [&](Type::Union& u) -> void
        {
          auto cur = get_curr_scope();
          for (
            ;
            cur && cur->m_types_table.find(u.name) == cur->m_types_table.end();
            cur = cur->prev
          );
          assert(cur != nullptr);
          auto sym = cur->m_sym_table.at(u.name);
          assert(sym.type.is_union_t());
          u = std::get<Type::Union>(sym.type.type);
        },
        [&](Type::Enum& e) -> void
        {
          auto cur = get_curr_scope();
          for (
            ;
            cur && cur->m_types_table.find(e.name) == cur->m_types_table.end();
            cur = cur->prev
          );
          assert(cur != nullptr);
          auto sym = cur->m_sym_table.at(e.name);
          assert(sym.type.is_enum_t());
          e = std::get<Type::Enum>(sym.type.type);
        },
        [&](Type::Func& f) -> void
        {
          for (auto& p : f.params) self(self, p);
          self(self, f.ret);
        }
      }, type->type);
    };

  analyze_type(analyze_type, ref.res_t);
  */
  /*
  std::visit(
    Utils::overloaded
    {
      [&](Type::Base& b) {},
      [&](Type::Ptr& p) { return analyze_type_ref(p.to); },
      [&](Type::Arr& a) { return analyze_type_ref(a.of); },
      [&](Type::Struct& s)
      {
        auto cur = get_curr_scope();
        for (
          ;
          cur && cur->m_types_table.find(s.name) == cur->m_types_table.end();
          cur = cur->prev
        );
        assert(cur != nullptr);
        auto sym = cur->m_sym_table.at(s.name);
        assert(sym.type.is_struct_t());
        s = std::get<Type::Struct>(sym.type.type);
      },
      [&](Type::Union& u)
      {
        auto cur = get_curr_scope();
        for (
          ;
          cur && cur->m_types_table.find(u.name) == cur->m_types_table.end();
          cur = cur->prev
        );
        assert(cur != nullptr);
        auto sym = cur->m_sym_table.at(u.name);
        assert(sym.type.is_union_t());
        u = std::get<Type::Union>(sym.type.type);
      },
      [&](Type::Enum& e)
      {
        auto cur = get_curr_scope();
        for (
          ;
          cur && cur->m_types_table.find(e.name) == cur->m_types_table.end();
          cur = cur->prev
        );
        assert(cur != nullptr);
        auto sym = cur->m_sym_table.at(e.name);
        assert(sym.type.is_enum_t());
        e = std::get<Type::Enum>(sym.type.type);
      }
    }, ref.type->type
  );
  */
    /*
  if (!is_valid_type(*type))
    Utils::panic("Invalid type encountered...");

  if (type->is_struct_t())
    populate_stc_t(std::get<Type::Struct>(type->type));
    */
}

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

void Sema::analyze_struct(const Node::Struct& strct)
{
  assert(false);
  /*
  if (
    get_curr_scope()->m_sym_table.find(strct.id.name) != get_curr_scope()->m_sym_table.end() &&
    get_curr_scope()->m_sym_table.at(strct.id.name).is_st()
  )
    Utils::panic("Struct redifinition for struct: '" + strct.id.name + "'");

  auto st = Type::Struct(strct.id.name);
  auto strct_ext = Sema::Symbol::StcExt();

  get_curr_scope()->m_sym_table.try_emplace(strct.id.name, Sema::Symbol(Type(st), strct_ext));
  g_anl.sym_table.try_emplace(strct.id.id, get_curr_scope()->m_sym_table.at(strct.id.name));
  get_curr_scope()->m_types_table.insert(st.name);

  for (auto& [type, member] : strct.members)
  {
    auto res_tp = std::visit(
      Utils::overloaded
      {
        [&](const Node::TypeDef& def) -> std::shared_ptr<Type>
        {
          analyze_type_def(def);
          def.res_t = std::visit(
            Utils::overloaded
            {
              [&](const std::shared_ptr<Node::Struct>& stc) -> std::shared_ptr<Type>
              { return std::make_shared<Type>(get_curr_scope()->m_sym_table.at(stc->id.name).type); },

              [&](const std::shared_ptr<Node::Union>& uni) -> std::shared_ptr<Type>
              { return std::make_shared<Type>(get_curr_scope()->m_sym_table.at(uni->id.name).type); },

              [&](const std::shared_ptr<Node::Enum>& enm) -> std::shared_ptr<Type>
              { return std::make_shared<Type>(get_curr_scope()->m_sym_table.at(enm->id.name).type); },
            }, def.type
          );
          return def.res_t;
        },
        [&](const Node::TypeRef& ref) -> std::shared_ptr<Type>
        { return ref.res_t; }
      }, type
    );
    // type = Node::TypeRef(res_tp);
  }

  auto resolve_type = [&](auto& self, auto& type) -> void
  {
    std::visit(
      Utils::overloaded
      {
        [&](Type::Base& b) {},
        [&](Type::Ptr& p) { self(self, *p.to); },
        [&](Type::Arr& a) { self(self, *a.of); },
        [&](Type::Struct& s)
        {
          auto cur = get_curr_scope();
          for (
            ;
            cur && cur->m_types_table.find(s.name) == cur->m_types_table.end();
            cur = cur->prev
          );
          assert(cur != nullptr);
          auto sym = cur->m_sym_table.at(s.name);
          assert(sym.type.is_struct_t());
          s.body = std::get<Type::Struct>(sym.type.type).body;
        },
        [&](Type::Union& u)
        {
          auto cur = get_curr_scope();
          for (
            ;
            cur && cur->m_types_table.find(u.name) == cur->m_types_table.end();
            cur = cur->prev
          );
          assert(cur != nullptr);
          auto sym = cur->m_sym_table.at(u.name);
          assert(sym.type.is_union_t());
          u.body = std::get<Type::Union>(sym.type.type).body;
        },
        [&](Type::Enum& e)
        {
          auto cur = get_curr_scope();
          for (
            ;
            cur && cur->m_types_table.find(e.name) == cur->m_types_table.end();
            cur = cur->prev
          );
          assert(cur != nullptr);
        },
        [&](auto& other) { assert(false && "const struct"); }
      }, const_cast<Type::TypeVar&>(type.type)
    );
  };

  for (auto& [type, member] : strct.members)
  {
    auto tp = Node::get_res_t(type);
    resolve_type(resolve_type, *tp);
    st.insert_mem(tp, member.name);
  }
  st.finish();

  auto& symt = get_curr_scope()->m_sym_table.at(strct.id.name);
  std::get<Type::Struct>(symt.type.type).size = st.size;

  auto& gsymt = g_anl.sym_table.at(strct.id.id);
  std::get<Type::Struct>(gsymt.type.type).size = st.size;

  for (auto& [type, field] : strct.members)
  {
    auto t = Node::get_res_t(type);
    if (!is_valid_type(*t))
      Utils::panic("Unknown type '" + Type::to_string(*t) + "' in struct definition");
  }
  */
}

void Sema::analyze_union(const Node::Union& un)
{
  if (
    get_curr_scope()->m_sym_table.find(un.id.name) != get_curr_scope()->m_sym_table.end() &&
    get_curr_scope()->m_sym_table.at(un.id.name).is_un()
  )
    Utils::panic("Union redifinition for union: '" + un.id.name + "'");

  auto unt = Type::Union(un.id.name);
  auto uni_ext = Sema::Symbol::UniExt();

  get_curr_scope()->m_sym_table.try_emplace(un.id.name, Sema::Symbol(Type(unt), uni_ext));
  g_anl.sym_table.try_emplace(un.id.id, get_curr_scope()->m_sym_table.at(un.id.name));
  get_curr_scope()->m_types_table.insert(unt.name);

  auto resolve_type = [&](auto& self, auto& type) -> void
  {
    std::visit(
      Utils::overloaded
      {
        [&](Type::Base& b) {},
        [&](Type::Ptr& p) { self(self, *p.to); },
        [&](Type::Arr& a) { self(self, *a.of); },
        [&](Type::Struct& s)
        {
          auto cur = get_curr_scope();
          for (
            ;
            cur && cur->m_types_table.find(s.name) == cur->m_types_table.end();
            cur = cur->prev
          );
          assert(cur != nullptr);
          auto sym = cur->m_sym_table.at(s.name);
          assert(sym.type.is_struct_t());
          s.body = std::get<Type::Struct>(sym.type.type).body;
        },
        [&](Type::Union& u)
        {
          auto cur = get_curr_scope();
          for (
            ;
            cur && cur->m_types_table.find(u.name) == cur->m_types_table.end();
            cur = cur->prev
          );
          assert(cur != nullptr);
          auto sym = cur->m_sym_table.at(u.name);
          assert(sym.type.is_union_t());
          u.body = std::get<Type::Union>(sym.type.type).body;
        },
        [&](Type::Enum& e)
        {
          auto cur = get_curr_scope();
          for (
            ;
            cur && cur->m_types_table.find(e.name) == cur->m_types_table.end();
            cur = cur->prev
          );
          assert(cur != nullptr);
        },
        [&](auto& other) { assert(false && "const struct"); }
      }, const_cast<Type::TypeVar&>(type.type)
    );
  };

  for (auto& [type, member] : un.members)
  {

    auto tp = Node::get_res_t(type);
    resolve_type(resolve_type, *tp);
    unt.insert_mem(tp, member.name);
    /*
    auto tp = std::visit(
      Utils::overloaded
      {
        [&](const Node::TypeDef& def) -> Type { return *def.res_t; },
        [&](const Node::TypeRef& ref) -> Type { return *ref.res_t; }
      }, type
    );
    */
  }
  unt.finish();

  auto& symt = get_curr_scope()->m_sym_table.at(un.id.name);
  std::get<Type::Union>(symt.type.type).size = unt.size;

  auto& gsymt = g_anl.sym_table.at(un.id.id);
  std::get<Type::Union>(gsymt.type.type).size = unt.size;

  for (auto& [type, field] : un.members)
  {
    auto t = Node::get_res_t(type);
    if (!is_valid_type(*t))
      Utils::panic("Unknown type '" + Type::to_string(*t) + "' in struct definition");
  }

}

void Sema::analyze_enum(const Node::Enum& enm)
{
  if (
    get_curr_scope()->m_sym_table.find(enm.id.name) != get_curr_scope()->m_sym_table.end() &&
    get_curr_scope()->m_sym_table.at(enm.id.name).is_en()
  )
    Utils::panic("Enum redifinition for enum: '" + enm.id.name + "'");

  auto enmt = Type::Enum(enm.id.name);
  auto enu_ext = Sema::Symbol::EnuExt();

  for (auto& asgn : enm.enums)
  {
    assert(asgn.val.has_value() && std::holds_alternative<std::shared_ptr<Node::Expr>>(asgn.val.value()));
    auto v = std::get<std::shared_ptr<Node::Expr>>(asgn.val.value());
    enmt.enums.try_emplace(asgn.id.name, std::move(v));
  }

  get_curr_scope()->m_sym_table.try_emplace(enm.id.name, Sema::Symbol(Type(enmt), enu_ext));
  g_anl.sym_table.try_emplace(enm.id.id, get_curr_scope()->m_sym_table.at(enm.id.name));
  get_curr_scope()->m_types_table.insert(enmt.name);
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
  // get_curr_scope()->prev->next = nullptr;
  m_scope_stack.pop_back();
}

/*
void Sema::register_struct_t(const Node::Struct& strct)
{
  auto st = Type::Struct(strct.id.name);
  auto strct_ext = Sema::Symbol::StcExt();

  get_curr_scope()->m_sym_table.try_emplace(strct.id.name, Sema::Symbol(Type(st), strct_ext));
  g_anl.sym_table.try_emplace(strct.id.id, get_curr_scope()->m_sym_table.at(strct.id.name));
  get_curr_scope()->m_types_table.insert(st.name);
}

void Sema::register_union_t(const Node::Union& un)
{
  auto uni = Type::Union(un.id.name);
  auto un_ext = Sema::Symbol::UniExt();

  get_curr_scope()->m_sym_table.try_emplace(un.id.name, Sema::Symbol(Type(uni), un_ext));
  g_anl.sym_table.try_emplace(un.id.id, get_curr_scope()->m_sym_table.at(un.id.name));
  get_curr_scope()->m_types_table.insert(uni.name);
}

void Sema::register_enum_t(const Node::Enum& en)
{
  auto enm = Type::Enum(en.id.name);
  auto en_ext = Sema::Symbol::EnuExt();

  get_curr_scope()->m_sym_table.try_emplace(en.id.name, Sema::Symbol(Type(enm), en_ext));
  g_anl.sym_table.try_emplace(en.id.id, get_curr_scope()->m_sym_table.at(en.id.name));
  get_curr_scope()->m_types_table.insert(enm.name);
}

bool Sema::is_type(const std::string& name)
{
  if (auto bt = Type::string_to_base_t(name)) return true;
  auto scp = get_curr_scope();
  for (; scp && !scp->m_sym_table.count(name); scp = scp->prev);
  return scp != nullptr;
}

Type Sema::get_type(const std::string& name)
{
  if (auto bt = Type::string_to_base_t(name)) return Type(*bt);
  auto scp = get_curr_scope();
  for (; scp && !scp->m_sym_table.count(name); scp = scp->prev);
  return scp->m_sym_table.at(name).type;
}
*/

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

/*
void Sema::populate_stc_t(Type::Struct& stc_t)
{
  auto cur = get_curr_scope();
  for (
    ;
    cur && cur->m_types_table.find(stc_t.name) == cur->m_types_table.end();
    cur = cur->prev
  );
  assert(cur != nullptr);
  auto sym = cur->m_sym_table.at(stc_t.name);
  assert(sym.type.is_struct_t());
  stc_t = std::get<Type::Struct>(sym.type.type);
}
*/

/*
void Sema::populate_stc(const Node::Ident& base, const Type::Struct& stc_t)
{
  bool ok = false;
  for (int i = (int)m_scope_stack.size() - 1; i >= 0; i--)
  {
    if (m_scope_stack[i]->m_sym_table.count(stc_t.name))
    {
      m_scope_stack.back()->m_sym_table.emplace(
        base.name,
        m_scope_stack[i]->m_sym_table.at(stc_t.name)
      );
      g_anl.sym_table.try_emplace(base.id, m_scope_stack.back()->m_sym_table.at(base.name));
      ok = true;
      break;
    }
  }
  assert(ok);
  for (auto& [member, type] : stc_t.types)
  {
    if (!type.is_struct_t()) {}
    else populate_stc(member);
  }
}
*/
