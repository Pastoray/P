#include "codegen.hpp"
#include "ir.hpp"
#include "utils.hpp"
#include <cassert>
#include <variant>

CodeGen::CodeGen(std::vector<IR::Instruct>& instructs)
      : m_instructs(std::move(instructs)), m_rbpoff(-16), m_reg_offset({}) { precomp_stack_size(IR::Label::create_code("_global"), m_instructs); }

void CodeGen::precomp_stack_size(const IR::Label& l, const std::vector<IR::Instruct>& inst)
{
  uint32_t sz = 0;
  for (auto& i : inst)
  {
    if (auto p = std::get_if<IR::Store>(&i))
      sz += IR::get_type(p->dest).size();
    
    else if (auto p = std::get_if<std::shared_ptr<IR::Func>>(&i))
      precomp_stack_size((*p)->label, (*p)->body);
  }
  m_ln_to_rsz.try_emplace(l.name, sz);
}

int32_t CodeGen::reg_offset(const IR::Reg& reg)
{
  if (m_reg_offset.find(reg) == m_reg_offset.end())
  {
    // m_rbpoff -= (int32_t)reg.reserve;
    // std::cout << reg.type << ": " << reg.type.size() << std::endl;
    m_rbpoff -= reg.type.size();
    m_reg_offset[reg] = m_rbpoff;
  }
  return m_reg_offset[reg];
}

Suffix CodeGen::sfx(const ASMOperand& oper)
{
  /*
  uint16_t sz = std::visit(
    [](auto&& arg) -> uint16_t
    {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, GPR>)
        return gpr_to_size(arg);
      else
        return arg.size();
    }, oper
  );
  return sfx(sz);
  */
  assert(false);
  return {};
}

Suffix CodeGen::sfx(OperandPtr oper)
{
  uint16_t sz = oper_sz(oper);
  return sfx(sz);
}

Suffix CodeGen::sfx(uint32_t sz)
{
  switch (sz)
  {
    case (1): return Suffix::B;
    case (2): return Suffix::W;
    case (4): return Suffix::L;
    case (8): return Suffix::Q;
    default:  return Suffix::Q; assert(false);
  }
}

uint32_t CodeGen::oper_sz(OperandPtr oper)
{
  struct Visitor
  {
    uint16_t operator()(const PreOper* po)
    { return po->size(); }

    uint16_t operator()(const IR::Operand* oper)
    { return IR::get_type(*oper).size(); }
  };
  return std::visit(Visitor{}, oper);
}

/*
Suffix CodeGen::get_com_sfx(OperandPtr oper1, OperandPtr oper2)
{
  auto suf1 = sfx(oper1);
  auto suf2 = sfx(oper2);

  if (suf1 == Suffix::Q || suf2 == Suffix::Q)
    return Suffix::Q;
  return Suffix::L;
}
*/

// Promote oper1.type to oper2.type
/*
void CodeGen::promote(PreOper& oper1, PreOper& oper2)
{
  if (oper2.indirect)
    oper1.resize(*oper2.type.inner(), m_text);
  else
    oper1.resize(oper2.type, m_text);
}
*/

/*
uint32_t CodeGen::coerce_common(PreOper& lhs, PreOper& rhs)
{
  uint32_t com_sz = std::max(lhs.size(), rhs.size());
  if (lhs.size() == rhs.size()) return com_sz;
  else if (lhs.size() > rhs.size()) rhs.resize(lhs.type, m_text);
  else lhs.resize(rhs.type, m_text);
  return com_sz;
}
*/


void CodeGen::MOV(OperandPtr lhs, OperandPtr rhs)
{
  m_text << "# prepare (start)\n\t";
  auto alhs = prepare_oper(lhs);
  auto arhs = prepare_dest(rhs);
  m_text << "# prepare (end)\n\t";

  bool lhsf = std::holds_alternative<Reg::FPR>(alhs.base_reg);
  bool rhsf = std::holds_alternative<Reg::FPR>(arhs.base_reg);

  if (lhsf && rhsf || lhsf && arhs.indirect)
  {
    if (arhs.size() == 8) m_text << "movsd" << " " << alhs << ", " << arhs << "\n\t";
    else m_text << "movss" << " " << alhs << ", " << arhs << "\n\t";
    return;
  }
  else if (lhsf || rhsf)
  {
    if (alhs.size() == 4) m_text << "movd" << " " << alhs << ", " << arhs << "\n\t";
    else m_text << "movq" << " " << alhs << ", " << arhs << "\n\t";
    return;
  }

  // promote(alhs, arhs);
  auto suf = sfx(arhs.size());
  if (alhs.indirect && arhs.indirect)
  {
    // assert(false && "Memory to memory move is not allowed");
    // required for *x = y;
    
    // m_text << "# EVIL MOV (" << alhs.size() << ", " << arhs.size() << ")\n\t";
    RegGuard tmp(pool, pool.alloc_any_gpr(arhs.size()));
    m_text << "mov" << suf << " " << alhs << ", " << tmp << "\n\t";
    m_text << "mov" << suf << " " << tmp << ", " << arhs << "\n\t";
    return;
  }
  else
  {
    // alhs.resize(arhs.type, m_text);
    m_text << "mov" << suf << " " << alhs << ", " << arhs << "\n\t";
  }
}


void CodeGen::ASG(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{
  m_text << "# asgn (start)\n\t";
  MOV(rhs, lhs);
  MOV(lhs, dest);
  m_text << "# asgn (end)\n\t";
  m_text << "\n\t";
}

void CodeGen::ADD(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{
  m_text << "# prepare (start)\n\t";
  auto alhs = prepare_oper(lhs);
  auto arhs = prepare_oper(rhs);

  bool lhsf = std::holds_alternative<Reg::FPR>(alhs.base_reg);
  bool rhsf = std::holds_alternative<Reg::FPR>(arhs.base_reg);

  if (lhsf && rhsf)
  {
    auto suf = sfx(dest);
    if (suf == Suffix::Q) m_text << "addsd" << " " << arhs << ", " << alhs << "\n\t";
    else if (suf == Suffix::L) m_text << "addss" << " " << arhs << ", " << alhs << "\n\t";
    else assert(false);
    MOV(&alhs, dest);
    return;
  }

  // uint32_t com_sz = coerce_common(alhs, arhs);
  // auto suf = sfx(com_sz);
  auto suf = sfx(dest);
  m_text << "# prepare (end)\n\t";

  m_text << "add" << suf << " " << arhs << ", " << alhs << "\n\t";
  MOV(&alhs, dest);
  m_text << "\n\t";

}

void CodeGen::SUB(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{
  m_text << "# prepare (start)\n\t";
  auto alhs = prepare_oper(lhs);
  auto arhs = prepare_oper(rhs);

  bool lhsf = std::holds_alternative<Reg::FPR>(alhs.base_reg);
  bool rhsf = std::holds_alternative<Reg::FPR>(arhs.base_reg);

  if (lhsf && rhsf)
  {
    auto suf = sfx(dest);
    if (suf == Suffix::Q) m_text << "subsd" << " " << arhs << ", " << alhs << "\n\t";
    else if (suf == Suffix::L) m_text << "subss" << " " << arhs << ", " << alhs << "\n\t";
    else assert(false);
    MOV(&alhs, dest);
    return;
  }

  // auto com_sz = coerce_common(alhs, arhs);
  // auto suf = sfx(com_sz);
  auto suf = sfx(dest);
  m_text << "# prepare (end)\n\t";

  m_text << "sub" << suf << " " << arhs << ", " << alhs << "\n\t";
  MOV(&alhs, dest);
  m_text << "\n\t";
}

void CodeGen::MUL(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{
  m_text << "# prepare (start)\n\t";
  auto alhs = prepare_oper(lhs);
  auto arhs = prepare_oper(rhs);

  // auto com_sz = coerce_common(alhs, arhs);
  // auto suf = sfx(com_sz);
  auto suf = sfx(dest);
  m_text << "# prepare (end)\n\t";

  m_text << "imul" << suf << " " << arhs << ", " << alhs << "\n\t";
  MOV(&alhs, dest);
  m_text << "\n\t";
}

void CodeGen::OR(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{
  m_text << "# prepare (start)\n\t";
  auto alhs = prepare_oper(lhs);
  auto arhs = prepare_oper(rhs);

  // auto com_sz = coerce_common(alhs, arhs);
  // auto suf = sfx(com_sz);
  auto suf = sfx(dest);
  m_text << "# prepare (end)\n\t";

  m_text << "or" << suf << " " << arhs << ", " << alhs << "\n\t";
  MOV(&alhs, dest);
  m_text << "\n\t";
}

void CodeGen::AND(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{
  m_text << "# prepare (start)\n\t";
  auto alhs = prepare_oper(lhs);
  auto arhs = prepare_oper(rhs);

  // auto com_sz = coerce_common(alhs, arhs);
  // auto suf = sfx(com_sz);
  auto suf = sfx(dest);
  m_text << "# prepare (end)\n\t";

  m_text << "and" << suf << " " << arhs << ", " << alhs << "\n\t";
  MOV(&alhs, dest);
  m_text << "\n\t";

}

void CodeGen::XOR(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{
  m_text << "# prepare (start)\n\t";
  auto alhs = prepare_oper(lhs);
  auto arhs = prepare_oper(rhs);

  // auto com_sz = coerce_common(alhs, arhs);
  // auto suf = sfx(com_sz);
  auto suf = sfx(dest);
  m_text << "# prepare (end)\n\t";

  m_text << "xor" << suf << " " << arhs << ", " << alhs << "\n\t";
  MOV(&alhs, dest);
  m_text << "\n\t";
}

void CodeGen::RSH(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{
  m_text << "# prepare (start)\n\t";
  auto alhs = prepare_oper(lhs);
  auto arhs = prepare_oper(rhs);

  // auto com_sz = coerce_common(alhs, arhs);
  // auto suf = sfx(com_sz);
  auto suf = sfx(dest);
  m_text << "# prepare (end)\n\t";

  m_text << "shr" << suf << " " << arhs << ", " << alhs << "\n\t";
  MOV(&alhs, dest);
  m_text << "\n\t";
}

void CodeGen::LSH(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{
  m_text << "# prepare (start)\n\t";
  auto alhs = prepare_oper(lhs);
  auto arhs = prepare_oper(rhs);
  
  // auto com_sz = coerce_common(alhs, arhs);
  // auto suf = sfx(com_sz);
  auto suf = sfx(dest);
  m_text << "# prepare (end)\n\t";

  m_text << "shl" << suf << " " << arhs << ", " << alhs << "\n\t";
  MOV(&alhs, dest);
  m_text << "\n\t";
}

void CodeGen::DIV(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{
  // Need to use rax & rdx for div regardless of availability
  auto rax64 = Reg::GPR::RAX;
  auto rdx64 = Reg::GPR::RDX;

  pool.lock(Reg::GPR::RAX);
  pool.lock(Reg::GPR::RDX);

  m_text << "# prepare (start)\n\t";
  auto alhs = prepare_oper(lhs);
  auto arhs = prepare_oper(rhs);
  auto adest = prepare_dest(dest);
  
  // auto com_sz = coerce_common(alhs, arhs);
  // auto suf = sfx(com_sz);
  auto suf = sfx(dest);
  m_text << "# prepare (end)\n\t";

  auto rax = (suf == Suffix::L ? to32(rax64) : rax64);
  auto rdx = (suf == Suffix::L ? to32(rdx64) : rdx64);

  m_text << "pushq " << rax64 << "\n\t";
  m_text << "pushq " << rdx64 << "\n\t";

  assert(std::holds_alternative<Reg::GPR>(arhs.base_reg));
  m_text << "pushq " << to64(std::get<Reg::GPR>(arhs.base_reg)) << "\n\t";
  
  m_text << "mov" << suf << " " << alhs << ", " << rax << "\n\t";
  m_text << "xor" << suf << " " << rdx << ", " << rdx << "\n\t";

  m_text << "div" << suf << " " << "(%rsp)" << "\n\t";
  m_text << "mov" << suf << " " << rax << ", " << adest << "\n\t";

  m_text << "addq $8, %rsp\n\t";
  m_text << "popq " << rdx64 << "\n\t";
  m_text << "popq " << rax64 << "\n\t";

  pool.unlock(rax64);
  pool.unlock(rdx64);
  m_text << "\n\t";
}

void CodeGen::IDX(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{
  m_text << "# idx (start)\n\t";
  ADD(lhs, rhs, dest);
  m_text << "# idx (end)\n\t";
}

void CodeGen::SETCC(const std::string& set_op, OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{
  m_text << "# prepare (start)\n\t";
  auto alhs = prepare_oper(lhs);
  auto arhs = prepare_oper(rhs);

  // auto com_sz = coerce_common(alhs, arhs);
  // auto suf = sfx(com_sz);
  auto suf = sfx(dest);
  m_text << "# prepare (end)\n\t";

  CMP(&alhs, &arhs);

  assert(std::holds_alternative<Reg::GPR>(alhs.base_reg));
  auto reg = std::get<Reg::GPR>(alhs.base_reg);
  m_text << set_op << " " << to8(reg) << "\n\t";
  m_text << "movzb" << suf << " " << to8(reg) << ", " << alhs << "\n\t";

  MOV(&alhs, dest);
  m_text << "\n\t";
}

void CodeGen::LT(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{ SETCC("setl", lhs, rhs, dest); }

void CodeGen::GT(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{ SETCC("setg", lhs, rhs, dest); }

void CodeGen::LTE(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{ SETCC("setle", lhs, rhs, dest); }

void CodeGen::GTE(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{ SETCC("setge", lhs, rhs, dest); }

void CodeGen::EQ(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{ SETCC("sete", lhs, rhs, dest); }

void CodeGen::NEQ(OperandPtr lhs, OperandPtr rhs, OperandPtr dest)
{ SETCC("setne", lhs, rhs, dest); }

void CodeGen::CMP(OperandPtr lhs, OperandPtr rhs)
{
  m_text << "# prepare (start)\n\t";
  auto alhs = prepare_oper(lhs);
  auto arhs = prepare_oper(rhs);

  // auto com_sz = coerce_common(alhs, arhs);
  // auto suf = sfx(com_sz);
  auto suf = sfx(lhs);
  m_text << "# prepare (end)\n\t";

  m_text << "cmp" << suf << " " << arhs << ", " << alhs << "\n\t";
  m_text << "\n\t";
}

void CodeGen::IMPC(OperandPtr dest, OperandPtr val, IR::ImpCast::CastOp op)
{
  m_text << "# cast (start) " << "(" << op << ")" << "\n\t";
  m_text << "# prepare (start)\n\t";
  auto adest = prepare_dest(dest);
  auto aval = prepare_oper(val);
  m_text << "# prepare (end)\n\t";
  auto unlock_reg = [&](const auto& reg) { pool.unlock(reg); };
  std::variant<Reg::GPR, Reg::FPR> tmp;
  if (adest.type.inner()->is_float())
    tmp = pool.alloc_any_fpr(adest.type.inner()->size());
  else
    tmp = pool.alloc_any_gpr(adest.type.inner()->size());
  auto poper = PreOper(*adest.type.inner(), tmp, false, 0, false, nullptr);

  if (aval.type.size() == adest.type.inner()->size())
  {
    m_text << "# CAST SAME SIZE (NOOP)\n\t";
    if (aval.type.is_float())
      m_text << "mov" << (adest.type.inner()->size() == 4 ? "ss" : "sd") << " " << aval << ", " << adest << "\n\t";
    else
      m_text << "mov" << sfx(&adest) << " " << aval << ", " << adest << "\n\t";
    std::visit(unlock_reg, tmp);
    // pool.unlock(tmp);
    return;
  }
  switch (op)
  {
    case (IR::ImpCast::CastOp::ZEXT):
      if (aval.type.size() == 4 && adest.type.inner()->size() == 8)
      {
        if (adest.indirect)
          m_text << "movl " << aval << ", " << adest << "\n\t";
        else
        {
          assert(std::holds_alternative<Reg::GPR>(adest.base_reg));
          auto reg = std::get<Reg::GPR>(adest.base_reg);
          m_text << "movl " << aval << ", " << Reg::to32(reg) << "\n\t";
        }
      }
      else
      {
        m_text << "movz" << sfx(&aval) << sfx(&adest) << " " << aval << ", " << poper << "\n\t";
        m_text << "mov" << sfx(&adest) << " " << poper << ", " << adest << "\n\t";
      }
      break;
    case (IR::ImpCast::CastOp::SEXT):
      m_text << "movs" << sfx(&aval) << sfx(&adest) << " " << aval << ", " << poper << "\n\t";
      m_text << "mov" << sfx(&adest) << " " << poper << ", " << adest << "\n\t";
      break;
    case (IR::ImpCast::CastOp::TRUNC):
      {
        assert(std::holds_alternative<Reg::GPR>(aval.base_reg));
        auto reg = std::get<Reg::GPR>(aval.base_reg);
        m_text << "mov" << sfx(&adest) << " " << Reg::as_sz(reg, adest.type.inner()->size()) << ", " << adest << "\n\t";
      }
      break;

      /*
      Logger::warn("NYI ImpCast");
      break;
      */
    case (IR::ImpCast::CastOp::SI2FP):
      {
        auto tmp = pool.alloc_any_fpr(8);
        auto poper = PreOper(Type(Type::Base::F64), tmp, false, 0, false, nullptr);
        m_text << "cvtsi2sd" << " " << aval << ", " << poper << "\n\t";
        m_text << "movsd" << " " << poper << ", " << adest << "\n\t";
        pool.unlock(tmp);
      }
      break;
    case (IR::ImpCast::CastOp::UI2FP):
      {
        auto tmp = pool.alloc_any_fpr(8);
        auto poper = PreOper(Type(Type::Base::F64), tmp, false, 0, false, nullptr);
        m_text << "cvtusi2sd" << " " << aval << ", " << poper << "\n\t";
        m_text << "movsd" << " " << poper << ", " << adest << "\n\t";
        pool.unlock(tmp);
      }
      break;
    case (IR::ImpCast::CastOp::FP2SI):
      {
        auto tmp = pool.alloc_any_gpr(8);
        auto poper = PreOper(Type(Type::Base::I64), tmp, false, 0, false, nullptr);
        m_text << "cvttsd2si" << " " << aval << ", " << poper << "\n\t";
        m_text << "movq" << " " << poper << ", " << adest << "\n\t";
        pool.unlock(tmp);
      }
      break;
    case (IR::ImpCast::CastOp::FP2UI):
      {
        auto tmp = pool.alloc_any_gpr(8);
        auto poper = PreOper(Type(Type::Base::U64), tmp, false, 0, false, nullptr);
        m_text << "vcvttsd2usi" << " " << aval << ", " << poper << "\n\t";
        m_text << "movq" << " " << poper << ", " << adest << "\n\t";
        pool.unlock(tmp);
      }
      break;
    case (IR::ImpCast::CastOp::FPEXT):
      {
        auto tmp = pool.alloc_any_fpr(8);
        auto poper = PreOper(Type(Type::Base::F64), tmp, false, 0, false, nullptr);
        m_text << "cvtss2sd" << " " << aval << ", " << poper << "\n\t";
        m_text << "movsd" << " " << poper << ", " << adest << "\n\t";
        pool.unlock(tmp);
        // m_text << "cvtss2sd" << " " << aval << ", " << adest << "\n\t";
      }
      break;
    case (IR::ImpCast::CastOp::FPTRUNC):
      {
        auto tmp = pool.alloc_any_fpr(8);
        auto poper = PreOper(Type(Type::Base::F32), tmp, false, 0, false, nullptr);
        m_text << "cvtsd2ss" << " " << aval << ", " << poper << "\n\t";
        m_text << "movss" << " " << poper << ", " << adest << "\n\t";
        pool.unlock(tmp);
      }
      break;
    case (IR::ImpCast::CastOp::BITCAST):
      m_text << "mov" << sfx(&adest) << " " << aval << ", " << adest << "\n\t";
      break;
    default: assert(0 && "Unknown cast");
  }
  m_text << "# cast (end)\n\t";
  m_text << "\n\t";
  std::visit(unlock_reg, tmp);
  // pool.unlock(tmp);
}

void CodeGen::gen_alloca(IR::Alloca& alc)
{
  {
    auto tmp = pool.alloc_any_gpr(8);
    // auto tp = Type(Type::Ptr(std::make_shared<Type>(Type::Base::I64)));
    // auto poper = PreOper(tp, tmp, true, 0, false, &pool);
    auto poper = PreOper(Type(Type::Base::I64), tmp, false, 0, false, &pool);
    auto val = IR::Operand(IR::Lit(4, Type(Type::Base::I64)));
    MOV(&val, &poper);
    auto dtmp = pool.alloc_any_gpr(8);

    // only reason this works is sizeof(ptr) == sizeof(i64)
    auto tp = Type(Type::Ptr(std::make_shared<Type>(Type::Base::I64)));
    auto dpoper = PreOper(tp, dtmp, false, 0, false, &pool);
    // auto dpoper = PreOper(Type(Type::Base::I64), dtmp, false, 0, false, &pool);
    for (auto& d : alc.dims)
    {
      IMPC(&dpoper, &d, IR::ImpCast::CastOp::ZEXT);
      MUL(&poper, &dpoper, &poper);
    }

    m_text << "subq " << poper << ", %rsp" << "\n\t";
    pool.unlock(dtmp);
    pool.unlock(tmp);
  }

  auto poper = PreOper(Type(Type::Base::I64), Reg::GPR::RSP, false, 0, false, &pool);
  MOV(&poper, &alc.dest);
  m_text << "\n\t";
}

void CodeGen::gen_allocc(IR::Allocc& alc)
{
  m_rbpoff -= alc.size;
}

void CodeGen::gen_impc(IR::ImpCast& impc)
{ IMPC(&impc.dest, &impc.val, impc.op); }

std::string CodeGen::gen_code()
{
  m_rodata << "\n.section .rodata\n\t";
  m_data << "\n.section .data\n\t";
  m_bss << "\n.section .bss\n\t";
  m_text << "\n.section .text\n\t";
  m_text << ".global _start\n\t";
  m_text << ".global main\n\t";
  m_text << "_start:\n\t";
  m_text << "pushq %rbp\n\t";
  m_text << "movq %rsp, %rbp\n\t";
  m_text << "subq $" << 32 * 32 << ", %rsp\n\t";

  for (auto& instr : m_instructs)
    gen_instr(instr);

  m_text << "call main\n\t";
  m_text << "movl $60, %eax\n\t" << "movl $0, %edi\n\t" << "syscall\n";
  return m_rodata.str() + m_data.str() + m_bss.str() + m_text.str();
}

void CodeGen::gen_instr(IR::Instruct& instr)
{
  std::visit(
    Utils::overloaded
    {
      [this](IR::GStore& i) { gen_gstore(i); },
      [this](IR::Store& i) { gen_store(i); },
      [this](IR::Binop& i) { gen_binop(i); },
      [this](IR::Unop& i) { gen_unop(i); },
      [this](IR::Jmp& i) { gen_jmp(i); },
      [this](IR::Jeq& i) { gen_jeq(i); },
      [this](IR::Jne& i) { gen_jne(i); },
      [this](IR::Label& i) { gen_label(i); },
      [this](IR::Call& i) { gen_call(i); },
      [this](IR::Param& i) { gen_param(i); },
      [this](IR::Ret& i) { gen_ret(i); },
      [this](IR::Nop& i) { gen_nop(i); },
      [this](IR::ImpCast& i) { gen_impc(i); },
      [this](IR::Alloca& i) { gen_alloca(i); },
      [this](IR::Allocc& i) { assert(false); },
      [this](std::shared_ptr<IR::Func>& i) { gen_fn(*i); },
      [](auto&) { std::cerr << "ERROR UNKNOWN INSTRUCTION" << std::endl; std::exit(-1); }
    }, instr
  );

  for (auto& [reg, cnt] : pool.gpr_usage)
    assert(cnt == 0 && "Detected register leak");

  for (auto& [reg, cnt] : pool.fpr_usage)
    assert(cnt == 0 && "Detected register leak");
}

PreOper CodeGen::prepare_oper(OperandPtr oper)
{
  return std::visit([&](auto&& arg) { return prepare_oper(arg); }, oper);
}

PreOper CodeGen::prepare_dest(OperandPtr oper)
{
  return std::visit([&](auto&& arg) { return prepare_dest(arg); }, oper);
}

PreOper CodeGen::prepare_oper(PreOper* oper)
{
  return std::move(*oper);
}

PreOper CodeGen::prepare_dest(PreOper* oper)
{
  return std::move(*oper);
}

PreOper CodeGen::prepare_dest(IR::Operand* oper)
{
  struct Visitor
  {
    CodeGen& gen;
    PreOper operator()(IR::Lit& lit)
    {
      assert(false);
    }
    PreOper operator()(IR::Reg& reg)
    {
      // auto tmp = gen.pool.alloc(8);
      auto tmp = Reg::GPR::R10;
      auto dest_t = Type(Type::Ptr(std::make_shared<Type>(reg.type)));
      gen.m_text << "lea" << Suffix::Q << " " << gen.format_operand(reg) << ", " << tmp << "\n\t";
      return PreOper(dest_t, tmp, true, 0, false, &gen.pool);
      // return PreOper(tmp, true, 0, true, &gen.pool, reg.osize, 8);
    }
    PreOper operator()(IR::Mem& mem)
    {
      // auto tmp1 = gen.pool.alloc(8);
      gen.m_text << "# mem (dest) (start)\n\t";
      auto tmp1 = Reg::GPR::R10;
      RegGuard tmp2(gen.pool, 8);
      RegGuard tmp3(gen.pool, 8);

      if (mem.base)
        gen.m_text << "mov" << Suffix::Q << " " << gen.format_operand(mem.base.value()) << ", " << tmp2 << "\n\t";

      if (mem.index)
      {
        // gen.m_text << "movl " << gen.format_operand(mem.index.value()) << ", " << tmp3 << "\n\t";
        // tmp3.reg = Reg::to64(tmp3);
        auto oper = IR::Operand(mem.index.value());
        auto tp = Type(Type::Ptr(std::make_shared<Type>(Type::Base::I64)));
        auto poper = PreOper(tp, tmp3, false, 0, false, &gen.pool);
        // auto poper = PreOper(Type(Type::Base::I64), tmp3, false, 0, false, &gen.pool);
        // gen.MOV(&oper, &poper);
        gen.IMPC(&poper, &oper, IR::ImpCast::CastOp::ZEXT);

      }

      std::stringstream ss;

      if (mem.disp) {}
      if (mem.base) ss << "(" << tmp2;
      if (mem.index) ss << ", " << tmp3;
      if (mem.scale) ss << ", " << *mem.scale;
      ss << ")";

      gen.m_text << "lea" << Suffix::Q << " " << ss.str() << ", " << tmp1 << "\n\t";
      auto dest_t = Type(Type::Ptr(std::make_shared<Type>(mem.type)));
      auto ret = PreOper(dest_t, tmp1, true, 0, false, &gen.pool);
      gen.m_text << "# mem (dest) (end)\n\t";
      return ret;
      // return PreOper(tmp1, true, 0, true, &gen.pool, mem.size, 8);
    }
    PreOper operator()(IR::Label& l)
    {
      // auto tmp = gen.pool.alloc(8);
      auto tmp = Reg::GPR::R10;
      gen.m_text << "lea" << Suffix::Q << " " << gen.format_operand(l) << ", " << tmp << "\n\t";
      auto dest_t = Type(Type::Ptr(std::make_shared<Type>(l.type)));
      return PreOper(dest_t, tmp, true, 0, false, &gen.pool);
      // return PreOper(tmp, true, 0, true, &gen.pool, l.size, 8);
    }
  };
  return std::visit(Visitor{ *this }, *oper);
}

PreOper CodeGen::prepare_oper(IR::Operand* oper)
{
  struct Visitor
  {
    CodeGen& gen;
    PreOper operator()(IR::Lit& lit)
    {
      // Suffix suf = sfx(lit.size);
      auto suf = sfx(lit.type.size());
      if (lit.type.is_float())
      {
        auto tmp = gen.pool.alloc_any_fpr(lit.type.size());
        if (lit.type.size() == 4) gen.m_text << "movss" << " " << gen.format_operand(lit) << ", " << tmp << "\n\t";
        if (lit.type.size() == 8) gen.m_text << "movsd" << " " << gen.format_operand(lit) << ", " << tmp << "\n\t";
        return PreOper(lit.type, tmp, false, 0, true, &gen.pool);
      }
      auto tmp = gen.pool.alloc_any_gpr(lit.type.size());
      gen.m_text << "mov" << suf << " " << gen.format_operand(lit) << ", " << tmp << "\n\t";
      return PreOper(lit.type, tmp, false, 0, true, &gen.pool);
      // return PreOper(tmp, false, 0, true, &gen.pool, lit.size, lit.size);
    }
    PreOper operator()(IR::Reg& reg)
    {
      Suffix suf = sfx(reg.type.size());
      if (reg.type.is_float())
      {
        auto tmp = gen.pool.alloc_any_fpr(reg.type.size());
        // gen.m_text << "movsd" << " " << gen.format_operand(reg) << ", " << tmp << "\n\t";
        if (reg.type.size() == 4) gen.m_text << "movss" << " " << gen.format_operand(reg) << ", " << tmp << "\n\t";
        if (reg.type.size() == 8) gen.m_text << "movsd" << " " << gen.format_operand(reg) << ", " << tmp << "\n\t";
        return PreOper(reg.type, tmp, false, 0, true, &gen.pool);
      }
      // gen.m_text << "# 1. SUF (" << suf << "), " << "OSIZE (" << reg.osize << "), " << "SIZE (" << reg.size << ").\n\t";
      auto tmp = gen.pool.alloc_any_gpr(reg.type.size());
      gen.m_text << "mov" << suf << " " << gen.format_operand(reg) << ", " << tmp << "\n\t";
      return PreOper(reg.type, tmp, false, 0, true, &gen.pool);
    }
    PreOper operator()(IR::Mem& mem)
    {
      gen.m_text << "# mem (oper) (start)\n\t";
      std::variant<Reg::GPR, Reg::FPR> tmp1;
      if (mem.type.is_float())
        tmp1 = gen.pool.alloc_any_fpr(mem.type.size());
      else
        tmp1 = gen.pool.alloc_any_gpr(mem.type.size());
      Suffix suf = sfx(mem.type.size());
      RegGuard tmp2(gen.pool, 8);
      RegGuard tmp3(gen.pool, 8);

      // We can call format_operand directly here because we know we're moving to GPRs
      if (mem.base)
      {
        // gen.m_text << "movq " << gen.format_operand(mem.base.value()) << ", " << tmp2 << "\n\t";
        auto oper = IR::Operand(mem.base.value());
        auto poper = PreOper(Type(Type::Base::I64), tmp2, false, 0, false, &gen.pool);
        gen.MOV(&oper, &poper);
      }

      if (mem.index)
      {
        // gen.m_text << "movq " << gen.format_operand(mem.index.value()) << ", " << tmp3 << "\n\t";
        auto oper = IR::Operand(mem.index.value());
        auto tp = Type(Type::Ptr(std::make_shared<Type>(Type::Base::I64)));
        auto poper = PreOper(tp, tmp3, false, 0, false, &gen.pool);
        // auto poper = PreOper(Type(Type::Base::I64), tmp3, false, 0, false, &gen.pool);
        // gen.MOV(&oper, &poper);
        gen.IMPC(&poper, &oper, IR::ImpCast::CastOp::ZEXT);
      }

      std::stringstream ss;
      if (mem.disp) {}
      if (mem.base) ss << tmp2;
      if (mem.index) ss << ", " << tmp3;
      if (mem.scale) ss << ", " << *mem.scale;

      auto print_reg = [&](const auto& reg) { gen.m_text << reg; };
      if (mem.type.is_float())
      {
        if (mem.type.size() == 4) gen.m_text << "movss";
        if (mem.type.size() == 8) gen.m_text << "movsd";
        // gen.m_text << "movsd";
      }
      else
        gen.m_text << "mov" << suf;

      gen.m_text << " (" << ss.str() << "), ";
      std::visit(print_reg, tmp1);
      gen.m_text << "\n\t";
      auto ret = PreOper(mem.type, tmp1, false, 0, true, &gen.pool);
      gen.m_text << "# mem (oper) (end)\n\t";
      return ret;
      // return PreOper(tmp1, false, 0, true, &gen.pool, mem.size, mem.size);
    }
    PreOper operator()(IR::Label& l)
    {
      // movq fn, %rax evaluates to pointer dereference
      // so we must explicitely leaq
      if (l.kind == IR::Label::Kind::CODE)
      {
        std::variant<Reg::GPR, Reg::FPR> tmp = gen.pool.alloc_any_gpr(8);
        gen.m_text << "leaq";
        auto print_reg = [&](const auto& reg) { gen.m_text << reg; };
        gen.m_text << " " << gen.format_operand(l) << "(%rip)" << ", ";
        std::visit(print_reg, tmp);
        gen.m_text << "\n\t";
        return PreOper(Type(Type::Ptr(std::make_shared<Type>(Type::Base::VOID))), tmp, false, 0, true, &gen.pool);
      }
      Suffix suf = sfx(l.type.size());
      std::variant<Reg::GPR, Reg::FPR> tmp;
      if (l.type.is_float())
        tmp = gen.pool.alloc_any_fpr(l.type.size());
      else
        tmp = gen.pool.alloc_any_gpr(l.type.size());
      // auto tmp = gen.pool.alloc_any_gpr(l.type.size());
      if (l.type.is_float())
      {
        if (l.type.size() == 4) gen.m_text << "movss";
        if (l.type.size() == 8) gen.m_text << "movsd";
        // gen.m_text << "movsd";
      }
      else
        gen.m_text << "mov" << suf;

      auto print_reg = [&](const auto& reg) { gen.m_text << reg; };
      gen.m_text << " " << gen.format_operand(l) << ", ";
      std::visit(print_reg, tmp);
      gen.m_text << "\n\t";
      return PreOper(l.type, tmp, false, 0, true, &gen.pool);
    }
  };
  return std::visit(Visitor{ *this }, *oper);
}

void CodeGen::gen_gstore(IR::GStore& gstore)
{
  auto dt = gstore.dest.type;
  switch (gstore.dest.kind)
  {
    case (IR::Label::Kind::BSS):
      {
        m_bss << gstore.dest.name << ": .space " << dt.size() << "\n\t";
        auto store = IR::Store(IR::Operand(gstore.dest), gstore.val);
        gen_store(store);
      }
      break;
    case (IR::Label::Kind::RODATA):
      {
        if (
          dt.is_ptr_t() &&
          dt.inner()->is_base_t() &&
          std::get<Type::Base>(dt.inner()->type) == Type::Base::CHAR
        ) { m_rodata << gstore.dest.name << ": .string " << "\"" << gstore.val << "\"" << "\n\t"; break; }
          
        // std::cout << dt << std::endl;
        assert(dt.is_base_t());
        std::string type_name = [&]()
        {
          auto bt = std::get<Type::Base>(dt.type);
          switch (bt)
          {
            case (Type::Base::CHAR): return std::string("byte");
            case (Type::Base::F32): return std::string("float");
            case (Type::Base::F64): return std::string("double");
            default: assert(false);
          }
          return std::string("");
        }();

        if (type_name == "byte")
        {
          m_rodata << gstore.dest.name << ": ." << type_name << " " << "\'" << gstore.val << "\'" << "\n\t";
        }
        else
        {
          m_rodata << gstore.dest.name << ": ." << type_name << " " << gstore.val << "\n\t";
        }
      }
      break;
    default: assert(false);
  }
  // change to actual size
  /*
  m_bss << gstore.dest.name << ": .space " << dt.size() << "\n\t";
  auto store = IR::Store(IR::Operand(gstore.dest), gstore.val);
  gen_store(store);
  */
}

void CodeGen::gen_store(IR::Store& store)
{
  /*
  const auto& val = format_operand(store.val);
  const auto& dest = format_operand(store.dest);
  */

  m_text << "# store (start)" << "\n\t";
  MOV(&store.val, &store.dest);
  m_text << "# store (end)" << "\n\t";
}

void CodeGen::gen_binop(IR::Binop& binop)
{
  m_text << "# binop (start)" << "\n\t";

  // assert(pool.available.size() == 5 && "Detected register leak");
  switch (binop.op)
  {
    case BinOp::ASG:
      ASG(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << rhs << ", %eax\n\t";
      m_text << "movl %eax, " << lhs << "\n\t"; // var
      m_text << "movl %eax, " << dest << "\n\t"; // ret
      */
      break;
    case BinOp::ADD:
      ADD(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "add " << rhs << ", %eax\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::SUB:
      SUB(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "sub " << rhs << ", %eax\n\t";
      m_text << "movl %eax, "  << dest << "\n\t";
      */
      break;
    case BinOp::MUL:
      MUL(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "movl " << rhs << ", %edx\n\t";
      // m_text << "mull " << rhs << "\n\t";
      m_text << "imull %edx, %eax\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::DIV:
      DIV(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "xor %edx, %edx\n\t";
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "movl " << rhs << ", %ecx" << "\n\t";
      m_text << "div %ecx\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::RSH:
      RSH(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "shr " << rhs << ", %eax\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::LSH:
      LSH(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "shl " << rhs << ", %eax\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::OR:
      OR(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "or " << rhs << ", %eax" "\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::AND:
      AND(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "and " << rhs << ", %eax" "\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::XOR:
      XOR(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "xor " << rhs << ", %eax" "\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::LT:
      LT(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "cmpl " << rhs << ", %eax\n\t";
      m_text << "setb %al\n\t";
      m_text << "movzx %al, %eax\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::GT:
      GT(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "cmpl " << rhs << ", %eax\n\t";
      m_text << "seta %al\n\t";
      m_text << "movzx %al, %eax\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::LTE:
      LTE(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "cmpl " << rhs << ", %eax\n\t";
      m_text << "setbe %al\n\t";
      m_text << "movzx %al, %eax\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::GTE:
      GTE(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "cmpl " << rhs << ", %eax\n\t";
      m_text << "setae %al\n\t";
      m_text << "movzx %al, %eax\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::EQ:
      EQ(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "cmpl " << rhs << ", %eax\n\t";
      m_text << "sete %al\n\t";
      m_text << "movzx %al, %eax\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::NEQ:
      NEQ(&binop.lhs, &binop.rhs, &binop.dest);
      /*
      m_text << "movl " << lhs << ", %eax\n\t";
      m_text << "cmpl " << rhs << ", %eax\n\t";
      m_text << "setne %al\n\t";
      m_text << "movzx %al, %eax\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
      break;
    case BinOp::IDX:
      IDX(&binop.lhs, &binop.rhs, &binop.dest);
      break;
    case BinOp::DOT:
      assert(false);
      // DOT(&binop.lhs, &binop.rhs, &binop.dest);
      break;
    default:
      Utils::panic("NYI BinOp");
      break;
  }
  m_text << "# binop (end)" << "\n\t";
}

void CodeGen::ADDR_OF(OperandPtr src, OperandPtr dest)
{
  auto asrc = prepare_dest(src);
  asrc.indirect = false;
  // asrc.type = *asrc.type.inner();

  // This is necessary because prepare_dest hardcodes R10
  // if we call another prepare_dest without saving this one
  // we would be modifying the content of this
  auto tmp = pool.alloc_any_gpr(8);
  auto poper = PreOper(asrc.type, tmp, false, 0, true, &pool);

  m_text << "movq " << asrc << ", " << poper << "\n\t";
  MOV(&poper, dest);
}

void CodeGen::DEREF(OperandPtr src, OperandPtr dest)
{
  auto asrc = prepare_oper(src);
  asrc.indirect = true;

  auto tmp = pool.alloc_any_gpr(8);
  auto poper = PreOper(*asrc.type.inner(), tmp, false, 0, true, &pool);

  m_text << "mov" << sfx(asrc.type.inner()->size()) << " " << asrc << ", " << poper << "\n\t";
  MOV(&poper, dest);
}

void CodeGen::INC(OperandPtr src, OperandPtr dest)
{
  auto adest = prepare_dest(dest);
  m_text << "inc" << sfx(adest.type.inner()->size()) << " " << adest << "\n\t";
}

void CodeGen::DEC(OperandPtr src, OperandPtr dest)
{
  auto adest = prepare_dest(dest);
  m_text << "dec" << sfx(adest.type.inner()->size()) << " " << adest << "\n\t";
}

void CodeGen::NOT(OperandPtr src, OperandPtr dest)
{
  auto adest = prepare_dest(dest);
  MOV(src, dest);
  m_text << "not" << sfx(adest.type.inner()->size()) << " " << adest << "\n\t";
}

void CodeGen::NEG(OperandPtr src, OperandPtr dest)
{
  auto adest = prepare_dest(dest);
  MOV(src, dest);
  m_text << "neg" << sfx(adest.type.inner()->size()) << " " << adest << "\n\t";
}

void CodeGen::gen_unop(IR::Unop& unop)
{
  /*
  const auto src = format_operand(unop.src);
  const auto dest = format_operand(unop.dest);
  */

  m_text << "# unop (start)" << "\n\t";

  switch (unop.op)
  {
    case UnOp::ADDR_OF:
      ADDR_OF(&unop.src, &unop.dest);
      /*
      m_text << "leaq " << src << ", %rax\n\t";
      m_text << "movq %rax, " << dest << "\n\t";
      */
      break;
    case UnOp::DEREF:
      DEREF(&unop.src, &unop.dest);
      /*
      m_text << "movl " << src << ", %edx\n\t";
      m_text << "movl (%edx), %eax\n\t";
      m_text << "movl %eax, " << dest << "\n\t";
      */
    case UnOp::INC:
      INC(&unop.src, &unop.dest);
      /*
      m_text << "incl " << dest << "\n\t";
      */
      break;
    case UnOp::DEC:
      DEC(&unop.src, &unop.dest);
      /*
      m_text << "decl " << dest << "\n\t";
      */
      break;
    case UnOp::NOT:
      NOT(&unop.src, &unop.dest);
      /*
      m_text << "notl " << dest << "\n\t";
      */
      break;
    case UnOp::NEG:
      NEG(&unop.src, &unop.dest);
  }
  m_text << "# unop (end)" << "\n\t";
}

void CodeGen::gen_jmp(IR::Jmp& jmp)
{
  m_text << "jmp " << format_operand(jmp.target) << "\n\t";
}

void CodeGen::gen_jeq(IR::Jeq& jeq)
{

  /*
  const auto& target = format_operand(jeq.target);
  const auto& lhs = format_operand(jeq.lhs);
  const auto& rhs = format_operand(jeq.rhs);
  */

  CMP(&jeq.lhs, &jeq.rhs);
  /*
  m_text << "movl " << lhs << ", %eax\n\t";
  m_text << "cmpl " << rhs << ", %eax\n\t";
  */
  m_text << "je " << format_operand(jeq.target) << "\n\t";
  // m_text << "je " << target << "\n\t";
}

void CodeGen::gen_jne(IR::Jne& jne)
{
  const auto& target = format_operand(jne.target);
  const auto& lhs = format_operand(jne.lhs);
  const auto& rhs = format_operand(jne.rhs);

  CMP(&jne.lhs, &jne.rhs);
  /*
  m_text << "movl " << lhs << ", %eax\n\t";
  m_text << "cmpl " << rhs << ", %eax\n\t";
  */
  m_text << "jne " << format_operand(jne.target) << "\n\t";
  // m_text << "jne " << target << "\n\t";
}

void CodeGen::gen_label(IR::Label& label)
{
  m_text << format_operand(label) << ":\n\t";
}


void CodeGen::gen_fn(IR::Func& fn)
{
  int prev_rbp = m_rbpoff;
  m_rbpoff = -16;
  gen_label(fn.label);
  m_text << "pushq %rbp\n\t";
  m_text << "movq %rsp, %rbp\n\t";
  m_text << "subq $" << 32 * 32 << ", %rsp\n\t";
  gen_params(fn.params); // moves params from register to stack
  for (auto& instr : fn.body)
    gen_instr(instr);
  // cleanup_stack();

  m_text << "leave\n\t"; // pop & restore rbp
  m_text << "ret\n\t"; // in-case of no return, we always allocate a register for a return value even for no ret routines
  m_rbpoff = prev_rbp;
}


void CodeGen::gen_params(std::vector<IR::Param>& params)
{
  std::vector<Reg::GPR> gp_regs = {
    Reg::GPR::RDI, Reg::GPR::RSI, Reg::GPR::RDX, Reg::GPR::RCX, Reg::GPR::R8, Reg::GPR::R9
  };
  std::vector<Reg::FPR> fp_regs = {
    Reg::FPR::XMM0, Reg::FPR::XMM1, Reg::FPR::XMM2, Reg::FPR::XMM3,
    Reg::FPR::XMM4, Reg::FPR::XMM5, Reg::FPR::XMM6, Reg::FPR::XMM7
  };

  int j = 0, k = 0;
  auto ctz_int = [&](auto&& self, const Type& tp, int cur, int from, int to) -> bool
  {
    if (tp.is_int() || tp.is_ptr_t()) return cur >= from && cur <= to; 
    if (auto st = std::get_if<Type::Struct>(&tp.type))
    {
      for (auto& [field, off] : st->body->offsets)
        if (cur + off >= from && cur + off <= to)
          if (self(self, *st->body->types.at(field), cur + off, from, to)) return true;
    }
    if (auto un = std::get_if<Type::Union>(&tp.type))
    {
      for (auto& [field, off] : un->body->offsets)
        if (cur + off >= from && cur + off <= to)
          if (self(self, *un->body->types.at(field), cur + off, from, to)) return true;
    }
    return false;
  };

  auto ctz_fp = [&](auto&& self, const Type& tp, int cur, int from, int to) -> bool
  {
    if (tp.is_float()) return cur >= from && cur <= to;
    if (auto st = std::get_if<Type::Struct>(&tp.type))
    {
      for (auto& [field, off] : st->body->offsets)
        if (cur + off >= from && cur + off <= to)
          if (self(self, *st->body->types.at(field), cur + off, from, to)) return true;
    }

    if (auto un = std::get_if<Type::Union>(&tp.type))
    {
      for (auto& [field, off] : un->body->offsets)
        if (cur + off >= from && cur + off <= to)
          if (self(self, *un->body->types.at(field), cur + off, from, to)) return true;
    }

    if (auto arr = std::get_if<Type::Arr>(&tp.type))
    {
      assert(false && "Not allowed to return array of any type");
    }
    return false;
  };

  std::vector<std::vector<std::variant<Reg::GPR, Reg::FPR>>> assigned_regs(params.size());
  std::stringstream temp;
  size_t tot_sz = 0;
  for (int i = 0; i < params.size(); i++)
  {
    auto arg_t = IR::get_type(params[i].reg);
    if (arg_t.size() > 16)
    {
    }
    else
    {
      struct Chunk
      {
        bool is_fp;
        int from;
      };

      std::vector<Chunk> chunks;
      for (int from = 0; from < arg_t.size(); from += 8)
      {
        int to = std::min(from + 7, (int)arg_t.size() - 1);
        
        bool has_int = ctz_int(ctz_int, arg_t, 0, from, to);
        bool has_fp  = ctz_fp(ctz_fp, arg_t, 0, from, to);
        bool is_fp = has_fp && !has_int;

        chunks.push_back({ is_fp, from });
      }

      int req_gp = 0, req_fp = 0;
      for (auto& c : chunks)
      {
        if (c.is_fp) req_fp++;
        else req_gp++;
      }

      bool fits = j + req_gp <= gp_regs.size() && k + req_fp <= fp_regs.size();
      auto tmp = IR::Operand(params[i].reg);
      // auto oper = prepare_oper(&tmp);
      if (fits)
      {
        for (auto& c : chunks)
        {
          if (c.is_fp) assigned_regs[i].emplace_back(fp_regs[k++]);
          else assigned_regs[i].emplace_back(gp_regs[j++]);
        }
      }
      else
      {
      }
    }
  }

  for (auto& regs : assigned_regs)
    for (auto& r : regs)
      std::visit([&](auto&& arg) { if (pool.contains(arg)) pool.lock(arg); }, r);

  int offset = 16;
  for (int i = 0; i < params.size(); i++)
  {
    auto param_t = IR::get_type(params[i].reg);
    auto tmp = IR::Operand(params[i].reg);
    auto oper = prepare_dest(&tmp);
    if (assigned_regs[i].empty())
    {
      int alloc_sz = (param_t.size() > 16 ? (param_t.size() + 15) & -16 : (param_t.size() + 7) & -8);
      auto temp = PreOper(param_t, pool.alloc_any_gpr(param_t.size()), false, 0, true, &pool);
      for (int p = 3, s = param_t.size(); s > 0;)
      {
        if (s - (1 << p) < 0) { p--; continue; }

        // m_text << "mov" << sfx(1 << p) << " " << (offset + s) << "(%rbp)" << ", " << oper << "\n\t";
        m_text << "mov" << sfx(1 << p) << " " << offset << "(%rbp)" << ", " << temp << "\n\t";
        m_text << "mov" << sfx(1 << p) << " " << temp << ", " << oper << "\n\t";
        oper.offset += 1 << p;
        s -= 1 << p;
        offset += 1 << 3;
      }
      // offset += alloc_sz;
    }
    else
    {
      size_t cur_sz = param_t.size();
      m_text << "# NOTE: using assigned registers\n\t";
      for (auto& r : assigned_regs[i])
      {
        if (cur_sz >= 8)
        {
          if (std::holds_alternative<Reg::FPR>(r)) m_text << "movsd" << " " << r << ", " << oper << "\n\t";
          else m_text << "movq" << " " << r << ", " << oper << "\n\t";
          oper.offset += 8;
          cur_sz -= 8;
        }
        else
        {
          for (size_t c : { 4, 2, 1 })
          {
            if (cur_sz <= 0) break;
            std::string instr;
            if (c == 1) instr = "movb";
            else if (c == 2) instr = "movw";
            else if (c == 4) instr = "movl";

            if (auto reg = std::get_if<Reg::GPR>(&r))
              m_text << instr << " " << as_sz(*reg, c) << ", " << oper << "\n\t";
            else if (auto reg = std::get_if<Reg::FPR>(&r))
              m_text << instr << " " << to128(*reg) << ", " << oper << "\n\t";
            oper.offset += c;
            cur_sz -= c;
          }
        }
      }
    }
  }

  for (auto& regs : assigned_regs)
    for (auto& r : regs)
      std::visit([&](auto&& arg) { if (pool.contains(arg)) pool.unlock(arg); }, r);
}

void CodeGen::gen_call(IR::Call& call)
{
  std::vector<Reg::GPR> gp_regs = {
    Reg::GPR::RDI, Reg::GPR::RSI, Reg::GPR::RDX, Reg::GPR::RCX, Reg::GPR::R8, Reg::GPR::R9
  };
  std::vector<Reg::FPR> fp_regs = {
    Reg::FPR::XMM0, Reg::FPR::XMM1, Reg::FPR::XMM2, Reg::FPR::XMM3,
    Reg::FPR::XMM4, Reg::FPR::XMM5, Reg::FPR::XMM6, Reg::FPR::XMM7
  };

  int j = 0, k = 0;
  auto ret_t = IR::get_type(call.dest);
  if (ret_t.size() > 16)
  {
    pool.lock(Reg::GPR::RDI);
    m_text << "subq" << " " << ret_t.size() << ", " << "%rsp\n\t";
    m_text << "movq %rsp, %rdi\n\t";
  }
  m_text << "movq %rsp, -1024(%rbp)\n\t";

  auto ctz_int = [&](auto&& self, const Type& tp, int cur, int from, int to) -> bool
  {
    if (tp.is_int() || tp.is_ptr_t()) return cur >= from && cur <= to; 
    if (auto st = std::get_if<Type::Struct>(&tp.type))
    {
      for (auto& [field, off] : st->body->offsets)
        if (cur + off >= from && cur + off <= to)
          if (self(self, *st->body->types.at(field), cur + off, from, to)) return true;
    }
    if (auto un = std::get_if<Type::Union>(&tp.type))
    {
      for (auto& [field, off] : un->body->offsets)
        if (cur + off >= from && cur + off <= to)
          if (self(self, *un->body->types.at(field), cur + off, from, to)) return true;
    }
    return false;
  };

  auto ctz_fp = [&](auto&& self, const Type& tp, int cur, int from, int to) -> bool
  {
    if (tp.is_float()) return cur >= from && cur <= to;
    if (auto st = std::get_if<Type::Struct>(&tp.type))
    {
      for (auto& [field, off] : st->body->offsets)
        if (cur + off >= from && cur + off <= to)
          if (self(self, *st->body->types.at(field), cur + off, from, to)) return true;
    }

    if (auto un = std::get_if<Type::Union>(&tp.type))
    {
      for (auto& [field, off] : un->body->offsets)
        if (cur + off >= from && cur + off <= to)
          if (self(self, *un->body->types.at(field), cur + off, from, to)) return true;
    }

    if (auto arr = std::get_if<Type::Arr>(&tp.type))
    {
      assert(false && "Not allowed to return array of any type");
    }
    return false;
  };

  std::vector<std::vector<std::variant<Reg::GPR, Reg::FPR>>> assigned_regs(call.args.size());
  size_t tot_sz = 0;
  for (int i = 0; i < call.args.size(); i++)
  {
    auto arg_t = IR::get_type(call.args[i]);
    if (arg_t.size() > 16)
    {
      tot_sz += (arg_t.size() + 15) & -16;
      // temp << "subq $15, %rsp\n\t";
      // temp << "andq $-16, %rsp\n\t";
      // auto sz = ((arg_t.size() + 15) & -16);
      // tot_sz += sz;
      /*
      for (int s = sz; s > 0; s -= 8)
      {
        auto oper = prepare_oper(&call.args[i]);
        temp << "pushq" << " " << oper << "\n\t";
        oper.offset += 8;
      }
      */
    }
    else
    {
      struct Chunk
      {
        bool is_fp;
        int from;
      };

      std::vector<Chunk> chunks;
      for (int from = 0; from < arg_t.size(); from += 8)
      {
        int to = std::min(from + 7, (int)arg_t.size() - 1);
        
        bool has_int = ctz_int(ctz_int, arg_t, 0, from, to);
        bool has_fp  = ctz_fp(ctz_fp, arg_t, 0, from, to);
        bool is_fp = has_fp && !has_int;

        chunks.push_back({ is_fp, from });
      }

      int req_gp = 0, req_fp = 0;
      for (auto& c : chunks)
      {
        if (c.is_fp) req_fp++;
        else req_gp++;
      }

      bool fits = j + req_gp <= gp_regs.size() && k + req_fp <= fp_regs.size();
      // auto oper = prepare_oper(&call.args[i]);
      if (fits)
      {
        for (auto& c : chunks)
        {
          if (c.is_fp) assigned_regs[i].emplace_back(fp_regs[k++]);
          else assigned_regs[i].emplace_back(gp_regs[j++]);
        }
        /*
        for (auto& c : chunks)
        {
          if (c.is_fp) temp << "movsd" << " " << oper << ", " << fp_regs[k++] << "\n\t";
          else temp << "movq" << " " << oper << ", " << gp_regs[j++] << "\n\t";
          oper.offset += 8;
        }
        */
      }
      else
      {
        tot_sz += (arg_t.size() + 7) & -8;
        /*
        for (int i = chunks.size() - 1; i >= 0; i--)
        {
          auto& c = chunks[i];
          oper.offset += c.from;
          if (c.is_fp)
          {
            tot_sz += 8;
            temp << "subq $8, %rsp\n\t";
            temp << "movsd" << " " << oper << ", " << "(%rsp)" << "\n\t";
          }
          else
          {
            tot_sz += 8;
            temp << "pushq" << " " << oper << "\n\t";
          }
          oper.offset -= c.from;
        }
        */
      }
    }
  }

  for (auto& regs : assigned_regs)
    for (auto& r : regs)
      std::visit([&](auto&& arg) { if (pool.contains(arg)) pool.lock(arg); }, r);

  auto rem = 16 - (tot_sz % 16);
  if (rem != 0)
    m_text << "subq" << " " << "$" << rem << ", " << "%rsp" << "\n\t";

  for (int i = call.args.size() - 1; i >= 0; i--)
  {
    auto arg_t = IR::get_type(call.args[i]);
    auto oper = prepare_oper(&call.args[i]);
    if (assigned_regs[i].empty())
    {
      for (int s = arg_t.size(); s > 0; s -= 8)
      {
        /*
        assert(reg != nullptr);
        m_text << "pushq" << " " << *reg << "\n\t";
        */
        m_text << "subq $8, %rsp\n\t";
        auto er = oper.effective_reg();
        if (auto reg = std::get_if<Reg::GPR>(&er))
          m_text << "movq" << " " << to64(*reg) << ", " << "(%rsp)" << "\n\t";
        else if (auto reg = std::get_if<Reg::FPR>(&er))
          m_text << "movq" << " " << to128(*reg) << ", " << "(%rsp)" << "\n\t";
        /*
        auto poper = PreOper(
          Type(Type::Ptr(std::make_shared<Type>(Type::Base::I64))),
          Reg::GPR::RSP,
          true, 0, false, nullptr
        );
        MOV(&oper, &poper);
        */
        oper.offset += 8;
      }
    }
    else
    {
      for (auto& r : assigned_regs[i])
      {
        if (std::holds_alternative<Reg::FPR>(r)) m_text << "movsd" << " " << oper << ", " << r << "\n\t";
        else
        {
          auto gpr = std::get<Reg::GPR>(oper.base_reg);
          m_text << "movq" << " " << to64(gpr) << ", " << r << "\n\t";
        }
        oper.offset += 8;
      }
    }
  }

  // m_text << temp.str();
  m_text << "call " << format_operand(call.callable) << "\n\t";
  m_text << "movq -1024(%rbp), %rsp\n\t";

  for (auto& regs : assigned_regs)
    for (auto& r : regs)
      std::visit([&](auto&& arg) { if (pool.contains(arg)) pool.unlock(arg); }, r);

  if (ret_t.size() > 16)
  {
    // pool.lock(Reg::GPR::RAX);
    auto adest = prepare_dest(&call.dest);
    int size = ret_t.size();
    auto poper = PreOper(ret_t, Reg::GPR::RDI, true, 0, false, nullptr);
    for (int p = 3; size > 0; size -= (1 << p))
    {
      while (p >= 0 && size - (1 << p) < 0) p--;
      MOV(&poper, &adest);
      adest.offset += 1 << p;
      poper.offset += 1 << p;
    }
    m_text << "addq" << " " << ret_t.size() << ", " << "%rsp\n\t";
    pool.unlock(Reg::GPR::RDI);
  }
  else
  {
    auto adest = prepare_dest(&call.dest);
    int i = 0, j = 0;
    std::vector<Reg::GPR> ret_gpr = { Reg::GPR::RAX, Reg::GPR::RDX };
    std::vector<Reg::FPR> ret_fpr = { Reg::FPR::XMM0, Reg::FPR::XMM1 };

    for (int from = 0; from < ret_t.size(); from += 8)
    {
      int to = std::min(from + 7, (int)ret_t.size() - 1);

      bool has_int = ctz_int(ctz_int, ret_t, 0, from, to);
      bool has_fp  = ctz_fp(ctz_fp, ret_t, 0, from, to);
      bool is_fp = has_fp && !has_int;

      if (is_fp)
        m_text << "movsd" << " " << ret_fpr[j++] << ", " << adest << "\n\t";
      else
        m_text << "movq" << " " << ret_gpr[i++] << ", " << adest << "\n\t";
      adest.offset += 8;
    }
  }
}

void CodeGen::gen_param(IR::Param& param)
{
  // param.reg;
  m_text << "# NYI (Param)\n\t";
  m_text << "nop\n\t";
  assert(false);
}

void CodeGen::gen_ret(IR::Ret& ret)
{
  // no need to lock XMM0/XMM1 as they are not in the pool
  pool.lock(Reg::GPR::RAX);
  pool.lock(Reg::GPR::RDX);
  auto aval = prepare_oper(&ret.ret);
  auto t = IR::get_type(ret.ret);
  int size = t.size();

  std::vector<Reg::GPR> ret_gpr = { Reg::GPR::RAX, Reg::GPR::RDX };
  std::vector<Reg::FPR> ret_fpr = { Reg::FPR::XMM0, Reg::FPR::XMM1 };
  int i = 0, j = 0;

  auto ctz_fp = [&](auto&& self, const Type& tp, int cur, int from, int to) -> bool
  {
    if (tp.is_float()) return cur >= from && cur <= to;
    if (auto st = std::get_if<Type::Struct>(&tp.type))
    {
      for (auto& [field, off] : st->body->offsets)
        if (cur + off >= from && cur + off <= to)
          if (self(self, *st->body->types.at(field), cur + off, from, to)) return true;
    }

    if (auto un = std::get_if<Type::Union>(&tp.type))
    {
      for (auto& [field, off] : un->body->offsets)
        if (cur + off >= from && cur + off <= to)
          if (self(self, *un->body->types.at(field), cur + off, from, to)) return true;
    }

    if (auto arr = std::get_if<Type::Arr>(&tp.type))
    {
      assert(false && "Not allowed to return array of any type");
      // return self(self, *arr->of, cur, from, to);
    }
    return false;
  };

  auto ctz_int = [&](auto&& self, const Type& tp, int cur, int from, int to) -> bool
  {
    if (tp.is_int() || tp.is_ptr_t()) return cur >= from && cur <= to; 
    if (auto st = std::get_if<Type::Struct>(&tp.type))
    {
      for (auto& [field, off] : st->body->offsets)
        if (cur + off >= from && cur + off <= to)
          if (self(self, *st->body->types.at(field), cur + off, from, to)) return true;
    }
    if (auto un = std::get_if<Type::Union>(&tp.type))
    {
      for (auto& [field, off] : un->body->offsets)
        if (cur + off >= from && cur + off <= to)
          if (self(self, *un->body->types.at(field), cur + off, from, to)) return true;
    }
    return false;
  };

  if (size <= 8)
  {
    std::variant<Reg::GPR, Reg::FPR> var;
    if (ctz_int(ctz_int, t, 0, 0, 7)) var = ret_gpr[i++];
    else var = ret_fpr[j++];

    auto poper = PreOper(t, var, false, 0, false, nullptr);
    MOV(&ret.ret, &poper);
  }
  else if (size <= 16)
  {
    std::variant<Reg::GPR, Reg::FPR> var1, var2;
    if (ctz_int(ctz_int, t, 0, 0, 7)) var1 = ret_gpr[i++];
    else var1 = ret_fpr[j++];

    if (ctz_int(ctz_int, t, 0, 8, 15)) var2 = ret_gpr[i++];
    else var2 = ret_fpr[j++];

    auto poper1 = PreOper(
      std::holds_alternative<Reg::GPR>(var1) ? Type(Type::Base::I64) : Type(Type::Base::F64),
      var1, false, 0, false, nullptr
    );
    auto poper2 = PreOper(
      std::holds_alternative<Reg::GPR>(var2) ? Type(Type::Base::I64) : Type(Type::Base::F64),
      var2, false, 0, false, nullptr
    );
    MOV(&aval, &poper1);
    aval.offset += 8;
    MOV(&aval, &poper2);
  }
  else
  {
    auto dest = PreOper(Type(Type::Ptr(std::make_shared<Type>(t))), Reg::GPR::RDI, true, 0, false, nullptr);
    for (int i = 3; i >= 0;)
    {
      if (size < (1 << i)) { i--; continue; }
      /*
      switch (i)
      {
        case (3): m_text << "movq" << " " << aval << " " << dest << "\n\t";
        case (2): m_text << "movl" << " " << aval << " " << dest << "\n\t";
        case (1): m_text << "movw" << " " << aval << " " << dest << "\n\t";
        case (0): m_text << "movb" << " " << aval << " " << dest << "\n\t";
        default: assert(false);
      }
      */
      MOV(&aval, &dest);
      dest.offset += 1 << i;
      aval.offset += 1 << i;
    }
    // force evacuation
    // auto tmp = pool.alloc_any_gpr(8);
    // auto poper_mv = PreOper(Type(Type::Base::I64), tmp, false, 0, false, nullptr);
    // MOV(&rax, &poper_mv);
    /*
    auto poper = PreOper(
      Type(Type::Ptr(std::make_shared<Type>(IR::get_type(ret.ret)))),
      Reg::GPR::RAX,
      false,
      0,
      false,
      nullptr
    );
    auto r = IR::Operand(IR::Reg(t));
    MOV(&ret.ret, &r);
    ADDR_OF(&r, &poper);
    */
  }
  // ADDR_OF(&ret.ret, &poper);
  // MOV(&poper_mv, &rax);
  pool.unlock(Reg::GPR::RDX);
  pool.unlock(Reg::GPR::RAX);
  // m_text << "movl " << format_operand(ret.ret) << ", %eax" << "\n\t";
  m_text << "leave\n\t";
  m_text << "ret\n\t";
}

void CodeGen::gen_nop(IR::Nop& nop)
{
  m_text << "nop\n\t";
}


