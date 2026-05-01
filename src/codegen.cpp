#include "codegen.hpp"
#include "ir.hpp"
#include <cassert>
#include <variant>

CodeGen::CodeGen(std::vector<IR::Instruct>& instructs)
      : m_instructs(std::move(instructs)), m_rbpoff(0), m_reg_offset({}) { precomp_stack_size(IR::Label::create_code("_global"), m_instructs); }

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

int32_t CodeGen::reg_offset(IR::Reg& reg)
{
  if (m_reg_offset.find(reg) == m_reg_offset.end())
  {
    // m_rbpoff -= (int32_t)reg.reserve;
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
    default: assert(false);
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

  // promote(alhs, arhs);
  auto suf = sfx(arhs.size());
  if (alhs.indirect && arhs.indirect)
  {
    // assert(false && "Memory to memory move is not allowed");
    // required for *x = y;
    
    // m_text << "# EVIL MOV (" << alhs.size() << ", " << arhs.size() << ")\n\t";
    RegGuard tmp(pool, pool.alloc_any(arhs.size()));
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
  m_text << "pushq " << to64(arhs.base_reg) << "\n\t";
  
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

  m_text << set_op << " " << to8(alhs.base_reg) << "\n\t";
  m_text << "movzb" << suf << " " << to8(alhs.base_reg) << ", " << alhs << "\n\t";

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
  m_text << "# cast (start)\n\t";
  m_text << "# prepare (start)\n\t";
  auto adest = prepare_dest(dest);
  auto aval = prepare_oper(val);
  m_text << "# prepare (end)\n\t";
  auto tmp = pool.alloc_any(8);
  auto poper = PreOper(adest.type, tmp, false, 0, false, nullptr);

  if (aval.type.size() == adest.type.size())
  {
    m_text << "# CAST SAME SIZE (NOOP)\n\t";
    m_text << "mov" << sfx(&adest) << " " << aval << ", " << adest << "\n\t";
    pool.unlock(tmp);
    return;
  }
  switch (op)
  {
    case (IR::ImpCast::CastOp::ZEXT):
      if (aval.type.size() == 4 && adest.type.size() == 8)
      {
        m_text << "movl " << aval << ", " << Reg::to32(adest.base_reg) << "\n\t";
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
      m_text << "mov" << sfx(&adest) << " " << aval << ", " << adest << "\n\t";
      break;
    case (IR::ImpCast::CastOp::SI2FP):
    case (IR::ImpCast::CastOp::UI2FP):
    case (IR::ImpCast::CastOp::FP2SI):
    case (IR::ImpCast::CastOp::FP2UI):
    case (IR::ImpCast::CastOp::FPEXT):
    case (IR::ImpCast::CastOp::FPTRUNC):
      Logger::warn("NYI ImpCast");
      break;
    case (IR::ImpCast::CastOp::BITCAST):
      m_text << "mov" << sfx(&adest) << " " << aval << ", " << adest << "\n\t";
      break;
    default: assert(0 && "Unknown cast");
  }
  m_text << "# cast (end)\n\t";
  m_text << "\n\t";
  pool.unlock(tmp);
}

void CodeGen::gen_alloca(IR::Alloca& alc)
{
  {
    auto tmp = pool.alloc_any(8);
    auto poper = PreOper(Type(Type::Base::I64), tmp, false, 0, false, &pool);
    auto val = IR::Operand(IR::Lit(4, Type(Type::Base::I64)));
    MOV(&val, &poper);
    auto dtmp = pool.alloc_any(8);
    auto dpoper = PreOper(Type(Type::Base::I64), dtmp, false, 0, false, &pool);
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

void CodeGen::gen_impc(IR::ImpCast& impc)
{ IMPC(&impc.dest, &impc.val, impc.op); }

std::string CodeGen::gen_code()
{
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
  return m_bss.str() + m_data.str() + m_text.str();
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
      [this](std::shared_ptr<IR::Func>& i) { gen_fn(*i); },
      [](auto&) { std::cerr << "ERROR UNKNOWN INSTRUCTION" << std::endl; std::exit(-1); }
    }, instr
  );
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
        auto poper = PreOper(Type(Type::Base::I64), tmp3, false, 0, false, &gen.pool);
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
      // This doesn't work for (jmp, jne, je) label(%rip)
      gen.m_text << "lea" << Suffix::Q << " " << gen.format_operand(l) << "(%rip), " << tmp << "\n\t";
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
      auto tmp = gen.pool.alloc_any(lit.type.size());
      gen.m_text << "mov" << suf << " " << gen.format_operand(lit) << ", " << tmp << "\n\t";
      return PreOper(lit.type, tmp, false, 0, true, &gen.pool);
      // return PreOper(tmp, false, 0, true, &gen.pool, lit.size, lit.size);
    }
    PreOper operator()(IR::Reg& reg)
    {
      Suffix suf = sfx(reg.type.size());
      // gen.m_text << "# 1. SUF (" << suf << "), " << "OSIZE (" << reg.osize << "), " << "SIZE (" << reg.size << ").\n\t";
      auto tmp = gen.pool.alloc_any(reg.type.size());
      gen.m_text << "mov" << suf << " " << gen.format_operand(reg) << ", " << tmp << "\n\t";
      return PreOper(reg.type, tmp, false, 0, true, &gen.pool);
    }
    PreOper operator()(IR::Mem& mem)
    {
      gen.m_text << "# mem (oper) (start)\n\t";
      Suffix suf = sfx(mem.type.size());
      auto tmp1 = gen.pool.alloc_any(mem.type.size());
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
        auto poper = PreOper(Type(Type::Base::I64), tmp3, false, 0, false, &gen.pool);
        // gen.MOV(&oper, &poper);
        gen.IMPC(&poper, &oper, IR::ImpCast::CastOp::ZEXT);
      }

      std::stringstream ss;
      if (mem.disp) {}
      if (mem.base) ss << tmp2;
      if (mem.index) ss << ", " << tmp3;
      if (mem.scale) ss << ", " << *mem.scale;

      gen.m_text << "mov" << suf << " (" << ss.str() << "), " << tmp1 << "\n\t";
      auto ret = PreOper(mem.type, tmp1, false, 0, true, &gen.pool);
      gen.m_text << "# mem (oper) (end)\n\t";
      return ret;
      // return PreOper(tmp1, false, 0, true, &gen.pool, mem.size, mem.size);
    }
    PreOper operator()(IR::Label& l)
    {
      Suffix suf = sfx(l.type.size());
      auto tmp = gen.pool.alloc_any(l.type.size());
      gen.m_text << "mov" << suf << " " << gen.format_operand(l) << ", " << tmp << "\n\t";
      return PreOper(l.type, tmp, false, 0, true, &gen.pool);
    }
  };
  return std::visit(Visitor{ *this }, *oper);
}

void CodeGen::gen_gstore(IR::GStore& gstore)
{
  // change to actual size
  m_bss << gstore.dest.name << ": .space " << gstore.dest.type.size() << "\n\t";
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

  for (auto& [reg, cnt] : pool.usage)
    assert(cnt == 0 && "Detected register leak");
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
  auto tmp = pool.alloc_any(8);
  auto poper = PreOper(asrc.type, tmp, false, 0, true, &pool);

  m_text << "movq " << asrc << ", " << poper << "\n\t";
  MOV(&poper, dest);
}

void CodeGen::DEREF(OperandPtr src, OperandPtr dest)
{
  auto asrc = prepare_oper(src);
  asrc.indirect = true;

  auto tmp = pool.alloc_any(8);
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
  m_text << "not" << sfx(adest.type.inner()->size()) << " " << adest << "\n\t";
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
}


void CodeGen::gen_params(std::vector<IR::Param>& params)
{
  std::vector<Reg::GPR> param_regs = { Reg::GPR::EDI, Reg::GPR::ESI, Reg::GPR::EDX, Reg::GPR::ECX, Reg::GPR::R8D, Reg::GPR::R9D };
  // assert(params.size() <= param_regs.size());
  if (params.empty())
    return;

  int len = std::min(param_regs.size(), params.size());
  for (int i = 0; i < len; i++)
  {
    if (pool.usage.count(param_regs[i])) pool.lock(param_regs[i]);
    auto poper = PreOper(params[i].reg.type, param_regs[i], false, 0, false, nullptr);
    IR::Operand oper = params[i].reg; 
    m_text << "# SMOVING PARAM\n\t";
    MOV(&poper, &oper);
    m_text << "# EMOVING PARAM\n\t";
  }

  int offset = 16;
  for (int i = len; i < params.size(); i++)
  {
    auto poper1 = PreOper(Type(Type::Ptr(std::make_shared<Type>(params[i].reg.type))), Reg::GPR::RBP, true, offset, false, nullptr);
    auto p = prepare_oper(&poper1);

    auto poper2 = PreOper(params[i].reg.type, Reg::GPR::R15, false, 0, false, nullptr);
    IR::Operand oper = params[i].reg;
    m_text << "# SMOVING PARAM\n\t";
    MOV(&poper1, &poper2);

    // m_text << "mov" << Suffix::Q << " " << poper1 << ", " << poper2 << "\n\t";
    MOV(&poper2, &oper);
    // MOV(&poper, &oper);
    m_text << "# EMOVING PARAM\n\t";
    offset += params[i].reg.type.size();
  }

  for (int i = 0; i < len; i++)
    if (pool.usage.count(param_regs[i])) pool.unlock(param_regs[i]);
}

void CodeGen::gen_call(IR::Call& call)
{
  std::vector<Reg::GPR> param_regs = { Reg::GPR::EDI, Reg::GPR::ESI, Reg::GPR::EDX, Reg::GPR::ECX, Reg::GPR::R8D, Reg::GPR::R9D };
  // assert(call.args.size() <= param_regs.size());

  int len = std::min(param_regs.size(), call.args.size());
  for (int i = 0; i < len; i++)
  {
    m_text << "pushq " << to64(param_regs[i]) << "\n\t";
    if (pool.usage.count(param_regs[i])) pool.lock(param_regs[i]);
  }

  for (int i = 0; i < len; i++)
  {
    auto poper = PreOper(IR::get_type(call.args[i]), param_regs[i], false, 0, false, nullptr);
    IR::Operand oper = call.args[i];
    MOV(&oper, &poper);
  }

  int acc = 0;
  for (int i = len; i < call.args.size(); i++)
    acc += IR::get_type(call.args[i]).size();
  
  m_text << "movq %rsp, -1024(%rbp)\n\t";
  m_text << "subq $15, %rsp\n\t";
  m_text << "andq $-16, %rsp\n\t";
  m_text << "subq $" << acc % 16 << ", %rsp" << "\n\t";
  m_text << "subq $" << acc << ", %rsp" << "\n\t";

  int offset = 0;
  for (int i = len; i < call.args.size(); i++)
  {
    auto poper = PreOper(Type(Type::Ptr(std::make_shared<Type>(IR::get_type(call.args[i])))), Reg::GPR::RSP, true, offset, false, &pool);
    IR::Operand oper = call.args[i];

    m_text << "# SMOVING PARAM\n\t";
    MOV(&oper, &poper);
    m_text << "# EMOVING PARAM\n\t";
    offset += IR::get_type(call.args[i]).size();
  }

  /*
  m_text << "subq $15, %rsp\n\t";
  m_text << "andq $-16, %rsp\n\t";
  */

  m_text << "call " << call.callable << "\n\t";

  m_text << "movq -1024(%rbp), %rsp\n\t";

  // m_text << "movq $0, " << acc << "\n\t";
  /*
  for (int i = len; i < call.args.size(); i++)
    m_text << "addq " << "$" << IR::get_type(call.args[i]).size() << ", " << acc << "\n\t";
  */
  // m_text << "addq " << acc << ", %rsp" << "\n\t";

  for (int i = len - 1; i >= 0; i--)
  {
    m_text << "popq " << to64(param_regs[i]) << "\n\t";
    if (pool.usage.count(param_regs[i])) pool.unlock(param_regs[i]);
  }

  const auto& dest = format_operand(call.dest);
  m_text << "movl %eax, " << dest << "\n\t";
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
  m_text << "movl " << format_operand(ret.ret) << ", %eax" << "\n\t";
  m_text << "leave\n\t";
  m_text << "ret\n\t";
}

void CodeGen::gen_nop(IR::Nop& nop)
{
  m_text << "nop\n\t";
}


