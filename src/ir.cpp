#include "ir.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <variant>

uint32_t IR::Reg::nid = 0;
uint32_t IR::Label::nid = 0;

IRGen::IRGen(std::vector<Node::Node>& nodes, Sema::Analysis& anl)
  : m_nodes(std::move(nodes)), m_index(0), m_anl(std::move(anl)), m_visitor(*this)
{ m_instructs = new std::vector<IR::Instruct>(); }

void IRGen::Visitor::visit(const Node::Func& func) { m_gen.gen_fn(func); }

void IRGen::Visitor::visit(const Node::Struct& strct) { m_gen.gen_struct(strct); }

void IRGen::Visitor::visit(const Node::BinExpr& bin_expr) { m_gen.gen_bin_expr(bin_expr); }

void IRGen::Visitor::visit(const Node::UnExpr& un_expr) { m_gen.gen_un_expr(un_expr); }

void IRGen::Visitor::visit(const Node::Ident& ident) { m_gen.gen_ident(ident); }

void IRGen::Visitor::visit(const Node::Int& int_) { m_gen.gen_int(int_); }

void IRGen::Visitor::visit(const Node::Asgn& asgn) { m_gen.gen_asgn(asgn); }

void IRGen::Visitor::visit(const Node::If& if_) { m_gen.gen_if(if_); }

void IRGen::Visitor::visit(const Node::For& for_) { m_gen.gen_for(for_); }

void IRGen::Visitor::visit(const Node::Continue& cnt) { m_gen.gen_continue(cnt); }

void IRGen::Visitor::visit(const Node::Break& brk) { m_gen.gen_break(brk); }

void IRGen::Visitor::visit(const Node::Ret& ret_) { m_gen.gen_ret(ret_); }

void IRGen::Visitor::visit(const Node::Call& call) { m_gen.gen_call(call); }

void IRGen::Visitor::visit(const Node::ExprStmt& expr_stmt) { m_gen.gen_expr_stmt(expr_stmt); }

void IRGen::match_types()
{
  auto lhs = m_stack.top();
  m_stack.pop();

  auto rhs = m_stack.top();
  m_stack.pop();

  auto lhs_t = IR::get_type(lhs);
  auto rhs_t = IR::get_type(rhs);

  if (lhs_t == rhs_t || lhs_t.is_arr_t() || rhs_t.is_ptr_t())
  {
    m_stack.push(rhs);
    m_stack.push(lhs);
    return;
  }

  if (lhs_t.rank() >= rhs_t.rank())
  {
    coerce(rhs, lhs_t);
    m_stack.push(lhs);
  }
  else
  {
    m_stack.push(rhs);
    coerce(lhs, rhs_t);
  }
}

void IRGen::coerce(const IR::Operand& oper, const Type& to)
{
  IR::Reg dest(to);
  IR::ImpCast::CastOp op = IR::ImpCast::resolv_cast_op(get_type(oper), to);
  m_stack.emplace(dest);
  m_instructs->emplace_back(IR::ImpCast(dest, oper, op));
}

void IRGen::gen_expr_stmt(const Node::ExprStmt& expr_stmt)
{
  expr_stmt.expr->accept(m_visitor);
  m_stack.pop();
}

void IRGen::gen_fn(const Node::Func& func)
{
  if (!func.scope.has_value())
    return;
  
  /* instead of flattening the function we keep it packed */
  // auto old_sym_table = sym_table;
  // auto old_reg_id = m_reg_id;

  // sym_table.clear();
  // m_reg_id = 1;

  /*
  IR::Label fn_lab(func.id.name, 0);
  IR::Label end_lab = new_label(0);
  */

  IR::Label fn_lab = IR::Label::create_code(func.id.name);
  // IR::Label end_lab = IR::Label::create_code(func.id.name + "_end");
  IR::Label end_lab = IR::Label::create_code();
  m_instructs->emplace_back(IR::Jmp(end_lab));

  // IR::Func fn(fn_lab);
  auto fn = std::make_shared<IR::Func>(fn_lab);
  // sym_table.emplace_back(); // function scope
  for (const Node::Param& prm : func.params)
  {
    // const IR::Reg r = new_reg(4);
    IR::Reg r(m_anl.sym_table.at(prm.id).type);
    {
      auto key = m_anl.sym_table.at(prm.id).id;
      aid_to_r.try_emplace(key, r);
      // m_instructs->emplace_back(IR::Store(aid_to_r.at(key), top));
    }

    const IR::Param param(r);
    fn->params.emplace_back(param);
    // sym_table.back().try_emplace(member.id, r);
    /*
    m_instructs->emplace_back(IR::Param{ r });
    */
  }

  m_instructs->emplace_back(fn);
  auto* prev_instructs = m_instructs;

  m_instructs = &fn->body;
  m_ctx_stack.push_back(ScopeCtx{ fn_lab, end_lab, ScopeCtx::Kind::FN });
  gen_scope(func.scope.value());
  m_ctx_stack.pop_back();

  m_instructs = prev_instructs;
  // sym_table = old_sym_table;
  // m_reg_id = old_reg_id;

  m_instructs->emplace_back(end_lab);

}

void IRGen::gen_struct(const Node::Struct& strct)
{
  /// ???
  /// for (auto member : strct.members)
  /// {
  ///   m_instructs->emplace_back(IR::Load{new_reg(), sym_table[member.id]});
  /// }
  /// strct.scope->accept(*this);
}

void IRGen::gen_bin_expr(const Node::BinExpr& bin_expr)
{
  bin_expr.lhs->accept(m_visitor);
  bin_expr.rhs->accept(m_visitor);

  {
    auto rhs = m_stack.top();
    m_stack.pop();

    auto lhs = m_stack.top();
    m_stack.pop();

    m_stack.push(rhs);
    m_stack.push(lhs);
  }

  match_types();

  IR::Operand lhs_oper = m_stack.top();
  m_stack.pop();

  IR::Operand rhs_oper = m_stack.top();
  m_stack.pop();

  if (IR::get_type(lhs_oper).is_arr_t() || IR::get_type(lhs_oper).is_ptr_t())
  {
    if (
      bin_expr.op == BinOp::ADD || bin_expr.op == BinOp::ASG_ADD ||
      bin_expr.op == BinOp::SUB || bin_expr.op == BinOp::ASG_SUB ||
      bin_expr.op == BinOp::IDX
    )
    {
      Type t = IR::get_type(lhs_oper);
      if (IR::get_type(lhs_oper).is_arr_t()) for (; t.is_arr_t(); t = *t.inner());
      else t = *t.inner();
      int stride = t.size();
      // m_instructs->emplace_back(IR::Store(scl, rhs_oper));
      coerce(rhs_oper, Type(Type::Base::I64));
      rhs_oper = m_stack.top();
      m_stack.pop();
      
      IR::Reg scl(IR::get_type(rhs_oper));

      m_instructs->emplace_back(IR::Binop(scl, rhs_oper, BinOp::MUL, IR::Lit(stride, Type(Type::Base::I64))));
      rhs_oper = scl;
    }
  }

  else if (IR::get_type(rhs_oper).is_arr_t() || IR::get_type(rhs_oper).is_ptr_t())
  {
    if (
      bin_expr.op == BinOp::ADD || bin_expr.op == BinOp::ASG_ADD ||
      bin_expr.op == BinOp::SUB || bin_expr.op == BinOp::ASG_SUB ||
      bin_expr.op == BinOp::IDX
    )
    {

      Type t = IR::get_type(rhs_oper);
      if (IR::get_type(rhs_oper).is_arr_t()) for (; t.is_arr_t(); t = *t.inner());
      else t = *t.inner();
      int stride = t.size();
      // m_instructs->emplace_back(IR::Store(scl, lhs_oper));
      coerce(lhs_oper, Type(Type::Base::I64));
      lhs_oper = m_stack.top();
      m_stack.pop();

      IR::Reg scl(IR::get_type(lhs_oper));

      m_instructs->emplace_back(IR::Binop(scl, lhs_oper, BinOp::MUL, IR::Lit(stride, Type(Type::Base::I64))));
      lhs_oper = scl;
    }
  }

  Type res_t = IR::get_type(lhs_oper);
  IR::Reg new_r(res_t);
  switch (bin_expr.op)
  {
    case BinOp::ADD:
      m_instructs->emplace_back(IR::Binop(new_r , lhs_oper, BinOp::ADD, rhs_oper));
      m_stack.emplace(new_r);
      break;
    case BinOp::ASG_ADD:
      m_instructs->emplace_back(
          IR::Binop{ lhs_oper , lhs_oper, BinOp::ADD, rhs_oper });
      m_instructs->emplace_back(
          IR::Store{ new_r, lhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::SUB:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::SUB, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::ASG_SUB:
      m_instructs->emplace_back(
          IR::Binop{ lhs_oper , lhs_oper, BinOp::SUB, rhs_oper });
      m_instructs->emplace_back(
          IR::Store{ new_r, lhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::MUL:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::MUL, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::ASG_MUL:
      m_instructs->emplace_back(
          IR::Binop{ lhs_oper , lhs_oper, BinOp::MUL, rhs_oper });
      m_instructs->emplace_back(
          IR::Store{ new_r, lhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::DIV:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::DIV, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::ASG_DIV:
      m_instructs->emplace_back(
          IR::Binop{ lhs_oper , lhs_oper, BinOp::DIV, rhs_oper });
      m_instructs->emplace_back(
          IR::Store{ new_r, lhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::ASG:
      m_instructs->emplace_back(
          IR::Binop{ new_r , lhs_oper, BinOp::ASG, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::LT:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::LT, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::GT:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::GT, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::LTE:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::LTE, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::GTE:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::GTE, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::EQ:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::EQ, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::NEQ:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::NEQ, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::RSH:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::RSH, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::LSH:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::LSH, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::AND:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::AND, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::OR:
      m_instructs->emplace_back(
          IR::Binop{ new_r, lhs_oper, BinOp::OR, rhs_oper });
      m_stack.emplace(new_r);
      break;
    case BinOp::IDX:
      if (auto* reg = std::get_if<IR::Reg>(&lhs_oper))
      {
        new_r.type = IR::get_type(rhs_oper);
        m_instructs->emplace_back(IR::Store(new_r, rhs_oper));
        auto& dims = r_to_arr_dims[*reg];
        IR::Reg stride(Type{ Type::Base::I64 });
        m_instructs->emplace_back(IR::Store(stride, IR::Lit(1, Type(Type::Base::I64))));

        for (int i = 1; i < dims.size(); i++)
        {
          coerce(dims[i], Type(Type::Base::I64));
          auto dim = m_stack.top();
          m_stack.pop();

          m_instructs->emplace_back(IR::Binop(stride, stride, BinOp::MUL, dim));
        }

        IR::Reg idx(Type{ Type::Base::I64 });
        m_instructs->emplace_back(IR::Binop(idx, new_r, BinOp::MUL, stride));

        if (res_t.inner()->is_arr_t())
        {
          IR::Reg r(*res_t.inner());
          m_instructs->emplace_back(IR::Binop(r, *reg, BinOp::ADD, idx));
          m_stack.emplace(r);
        }
        else
          m_stack.emplace(IR::Mem(*res_t.inner(), std::nullopt, *reg, idx, IR::Lit(1, Type(Type::Base::I32))));
      }
      else if (auto* mem = std::get_if<IR::Mem>(&lhs_oper))
      {
        new_r.type = IR::get_type(rhs_oper);
        m_instructs->emplace_back(IR::Store(new_r, rhs_oper));
        auto& new_t = mem->type;
        auto& base = mem->base.value();
        auto& idx = mem->index.value();
        auto& scale = mem->scale.value();
        auto& dims = r_to_arr_dims[base];

        {
          int d1 = 0;
          for (auto t = base.type; t.is_arr_t(); t = *t.inner()) d1++;

          int d2 = 0;
          for (auto t = new_t; t.is_arr_t(); t = *t.inner()) d2++;
          std::cout << "DIMEN DIFF = " << d1 << " - " << d2 << std::endl;

          IR::Reg stride(Type{ Type::Base::I64 });
          m_instructs->emplace_back(IR::Store(stride, IR::Lit(1, Type(Type::Base::I64))));
          for (int i = d1 - d2 + 1; i < dims.size(); i++)
          {
            coerce(dims[i], Type(Type::Base::I64));
            auto dim = m_stack.top();
            m_stack.pop();

            m_instructs->emplace_back(IR::Binop(stride, stride, BinOp::MUL, dim));
          }

          m_instructs->emplace_back(IR::Binop(new_r, new_r, BinOp::MUL, stride));
          m_instructs->emplace_back(IR::Binop(idx, idx, BinOp::ADD, new_r));
          new_t = *new_t.inner();
          // scale = IR::Lit(new_t.size(), Type(Type::Base::I32));
          scale = IR::Lit(1, Type(Type::Base::I32));
        }

        m_stack.emplace(lhs_oper);
      }
      else assert(0);
      break;
    default:
      Logger::warn("IR BinExpr fell through...");
      break;
  }
}

void IRGen::gen_un_expr(const Node::UnExpr& un_expr)
{
  un_expr.expr->accept(m_visitor);
  IR::Operand oper = m_stack.top();
  m_stack.pop();

  IR::Reg new_r(IR::get_type(oper));
  switch (un_expr.op)
  {
    case UnOp::ADDR_OF:
      new_r.type = Type(Type::Ptr(std::make_shared<Type>(new_r.type)));
      m_instructs->emplace_back(
          IR::Unop{ new_r, oper, UnOp::ADDR_OF });
      m_stack.emplace(new_r);
      break;
    case UnOp::DEREF:
      m_stack.emplace(
        IR::Mem(*IR::get_type(oper).inner(), std::nullopt, std::get<IR::Reg>(oper), std::nullopt, std::nullopt)
      );
      break;
    case UnOp::INC:
      if (un_expr.pref)
      {
        m_instructs->emplace_back(
            IR::Unop{ oper, oper, UnOp::INC });
        m_instructs->emplace_back(
            IR::Store{ new_r, oper });
      }
      else
      {
        m_instructs->emplace_back(
            IR::Store{ new_r, oper });
        m_instructs->emplace_back(
            IR::Unop{ oper, oper, UnOp::INC });
      }
      m_stack.emplace(new_r);
      break;
    case UnOp::DEC:
      if (un_expr.pref)
      {
        m_instructs->emplace_back(
            IR::Unop{ oper, oper, UnOp::DEC });
        m_instructs->emplace_back(
            IR::Store{ new_r, oper });
      }
      else
      {
        m_instructs->emplace_back(
            IR::Store{ new_r, oper });
        m_instructs->emplace_back(
            IR::Unop{ oper, oper, UnOp::DEC });
      }
      m_stack.emplace(new_r);
      break;
    case UnOp::NOT:
      m_instructs->emplace_back(
        IR::Unop{ new_r, oper, UnOp::NOT });
      m_stack.emplace(new_r);
      break;
    case UnOp::NEG:
      m_instructs->emplace_back(
        IR::Unop{ new_r, oper, UnOp::NEG });
      m_stack.emplace(new_r);
      break;
  }
}

void IRGen::gen_ident(const Node::Ident& ident)
{
  auto key = m_anl.sym_table.at(ident.id).id;
  auto it1 = aid_to_r.find(key);
  auto it2 = aid_to_l.find(key);
  if (it1 == aid_to_r.end() && it2 == aid_to_l.end())
  {
    assert(m_anl.sym_table.count(ident.id) && "Mapping from Ident.id to Analysis.id missing");
    assert(false);
    // aid_to_r.try_emplace(m_anl.sym_table.at(ident.id).id, IR::Reg(m_anl.sym_table.at(ident.id).type));
  }
  if (it2 == aid_to_l.end())
    m_stack.emplace(aid_to_r.at(key));

  if (it1 == aid_to_r.end())
    m_stack.emplace(aid_to_l.at(key));

  /*
  m_stack.emplace(m_anl);
  if (m_g_table.find(ident.id) != m_g_table.end())
  {
    m_stack.emplace(m_g_table.at(ident.id));
  }
  else if (auto reg = find_local_sym(ident.id))
  {
    m_stack.emplace(reg.value());
  }
  else
  {
    assert(false);
  }
  */
}

void IRGen::gen_int(const Node::Int& int_)
{
  m_stack.emplace(IR::Lit(int_.val, Type(Type::Base::I32)));
}

void IRGen::gen_asgn(const Node::Asgn& asgn)
{
  if (auto* arr = std::get_if<Type::Arr>(&asgn.type->type))
  {
    auto dest = IR::Reg(*asgn.type);
    auto sym = m_anl.sym_table.at(asgn.id.id);
    auto alloca = IR::Alloca(dest, {});
    
    auto cur_t = arr;
    while (cur_t)
    {
      auto d = std::static_pointer_cast<Node::Expr>(cur_t->size);
      d->accept(m_visitor);
      auto top = m_stack.top();
      alloca.dims.push_back(top);
      m_stack.pop();
      cur_t = std::get_if<Type::Arr>(&cur_t->of->type);
    }

    std::reverse(alloca.dims.begin(), alloca.dims.end());
    r_to_arr_dims.try_emplace(dest, alloca.dims);
    aid_to_r.try_emplace(sym.id, dest);
    auto top = IR::Lit(0, *asgn.type);
    m_instructs->emplace_back(IR::Store(aid_to_r.at(sym.id), top));
    m_instructs->emplace_back(alloca);
    return;
  }
  if (asgn.val)
    std::visit(
      Utils::overloaded
      {
        [this, &asgn](const std::shared_ptr<Node::Expr>& expr)
        {
          expr->accept(m_visitor);
          auto sym = m_anl.sym_table.at(asgn.id.id);
          if (auto* ext = std::get_if<Sema::Symbol::VarExt>(&sym.ext))
          {
            auto top = m_stack.top();
            if (ext->is_global)
            {
              m_instructs->emplace_back(IR::GStore(IR::Label::create_data(asgn.id.name, IR::get_type(top)), top));
              auto lbl = IR::Label::create_data(asgn.id.name, IR::get_type(top));
              aid_to_l.try_emplace(sym.id, lbl);
            }
            else
            {
              aid_to_r.try_emplace(sym.id, IR::Reg(IR::get_type(top)));
              m_instructs->emplace_back(IR::Store(aid_to_r.at(sym.id), top));
            }
          }
          m_stack.pop();
        },

        [this, &asgn](const std::shared_ptr<Node::Decl>& decl)
        { assert(false); }
      }, *asgn.val
    );
  else
    if (auto* ext = std::get_if<Sema::Symbol::VarExt>(&m_anl.sym_table.at(asgn.id.id).ext))
    {
      auto top = IR::Lit(0, *asgn.type);
      if (ext->is_global)
      {
        m_instructs->emplace_back(IR::GStore(IR::Label::create_data(asgn.id.name, IR::get_type(top)), top));
        auto key = m_anl.sym_table.at(asgn.id.id).id;
        auto lbl = IR::Label::create_data(asgn.id.name, IR::get_type(top));
        aid_to_l.try_emplace(key, lbl);
      }
      else
      {
        auto key = m_anl.sym_table.at(asgn.id.id).id;
        aid_to_r.try_emplace(key, IR::Reg(IR::get_type(top)));
        m_instructs->emplace_back(IR::Store(aid_to_r.at(key), top));
      }
    }
  /*
  /// if expr
  asgn.val->accept(*this);
  sym_table[asgn.id.id] = m_stack.top();
  m_stack.pop();
  /// if decl
  /// ???
  /// function:
  IR::Label end_label = new_label();
  m_instructs->emplace_back(IR::Jmp{ end_label });
  m_instructs->emplace_back(IR::Label{ asgn.id.id });
  asgn.val->accept(*this);
  m_instructs->emplace_back(end_label);
  /// struct:
  m_instructs->emplace_back(IR::Alloc{ new_reg(), asgn.id.id });
  */
}

void IRGen::gen_if(const Node::If& if_)
{
  IR::Label end_label = IR::Label::create_code();

  if_.cond->accept(m_visitor);
  IR::Operand cond = m_stack.top();
  m_stack.pop();

  IR::Label if_label = IR::Label::create_code();
  IR::Label else_label = IR::Label::create_code();

  m_instructs->emplace_back(IR::Jne{ cond, IR::Lit(0, Type(Type::Base::I32)), if_label });
  m_instructs->emplace_back(IR::Jmp{ else_label });
  m_instructs->emplace_back(if_label);

  m_ctx_stack.push_back(ScopeCtx{ if_label, else_label,  ScopeCtx::Kind::IF });
  gen_scope(*if_.scope);
  m_ctx_stack.pop_back();

  m_instructs->emplace_back(IR::Jmp{ end_label });
  m_instructs->emplace_back(else_label);

  Node::If curr_if = if_;
  while (curr_if.elif->has_value())
  {
    curr_if = curr_if.elif->value();
    curr_if.cond->accept(m_visitor);
    IR::Operand new_cond = m_stack.top();
    m_stack.pop();

    IR::Label new_if_label = IR::Label::create_code();
    IR::Label new_else_label = IR::Label::create_code();

    m_instructs->emplace_back(IR::Jne{ new_cond, IR::Lit(0, Type(Type::Base::I32)), new_if_label });
    m_instructs->emplace_back(IR::Jmp{ new_else_label });
    m_instructs->emplace_back(new_if_label);

    m_ctx_stack.push_back(ScopeCtx{ new_if_label, new_else_label, ScopeCtx::Kind::IF });
    gen_scope(*curr_if.scope);
    m_ctx_stack.pop_back();
    // curr_if.scope->accept(*this);

    m_instructs->emplace_back(IR::Jmp{ end_label });
    m_instructs->emplace_back(new_else_label);
  }
  m_instructs->emplace_back(end_label);
}

void IRGen::gen_for(const Node::For& for_)
{
  IR::Label start_label = IR::Label::create_code();
  IR::Label end_label = IR::Label::create_code();
  IR::Label adv_label = IR::Label::create_code();

  if (for_.init) (*for_.init)->accept(m_visitor);

  m_instructs->emplace_back(start_label);
  IR::Operand cond = IR::Lit(1, Type(Type::Base::I32));
  if (for_.cond) 
  {
    (*for_.cond)->accept(m_visitor);
    cond = m_stack.top();
    m_stack.pop();
  }

  m_instructs->emplace_back(IR::Jeq{ cond, IR::Lit(0, Type(Type::Base::I32)), end_label });
  m_ctx_stack.push_back(ScopeCtx{ adv_label, end_label, ScopeCtx::Kind::LOOP });
  gen_scope(*for_.scope);
  m_ctx_stack.pop_back();

  m_instructs->emplace_back(adv_label);
  if (for_.adv)
  {
    (*for_.adv)->accept(m_visitor);
    m_stack.pop();
  }

  m_instructs->emplace_back(IR::Jmp{ start_label });
  m_instructs->emplace_back(end_label);
}

void IRGen::gen_continue(const Node::Continue& cnt)
{
  int i = m_ctx_stack.size() - 1;
  for (; i >= 0 && m_ctx_stack[i].kind != ScopeCtx::Kind::LOOP; i--);
  assert(i > -1);
  m_instructs->emplace_back(IR::Jmp{ m_ctx_stack[i].start });
}

void IRGen::gen_break(const Node::Break& brk)
{
  int i = m_ctx_stack.size() - 1;
  for (; i >= 0 && m_ctx_stack[i].kind != ScopeCtx::Kind::LOOP; i--);
  assert(i > -1);
  m_instructs->emplace_back(IR::Jmp{ m_ctx_stack[i].end });
}

void IRGen::gen_ret(const Node::Ret& ret_)
{
  if (ret_.ret_val.has_value())
  {
    ret_.ret_val.value()->accept(m_visitor);
    const IR::Operand ret_v = m_stack.top();
    m_stack.pop();

    m_instructs->emplace_back(IR::Ret(ret_v));
  }
  else
  {
    /// ???
    /// Should provide some sort of primitive for void returns
    /// something like a VOID macro might work
    /// we might skip this for now until types are fully supported
    m_instructs->emplace_back(IR::Ret(IR::Lit(0, Type(Type::Base::I32))));
  }
}

void IRGen::gen_call(const Node::Call& call)
{
  IR::Reg ret_reg((Type(Type::Base::I32)));
  IR::Call ir_call(ret_reg);
  ir_call.callable = call.callable.name;
  for (auto& expr : call.args)
  {
    expr->accept(m_visitor);
    auto top = m_stack.top();
    m_stack.pop();
    ir_call.args.emplace_back(top);
  }
  m_stack.emplace(ret_reg);
  m_instructs->emplace_back(ir_call);
}

void IRGen::gen_scope(const Node::Scope& scope)
{
  for (auto& term : scope.terms)
  { std::visit([this](const auto& t) { t->accept(m_visitor); }, term); }
}

std::vector<IR::Instruct> IRGen::gen()
{
  for (auto& v : m_nodes)
    std::visit([this](const auto& t) { t->accept(m_visitor); }, v);
  return *m_instructs;
}
