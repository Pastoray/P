#ifndef CODEGEN_H
#define CODEGEN_H
#include "ir.hpp"
#include "utils.hpp"

#include <cassert>
#include <cstdint>
#include <variant>
#include <vector>
#include <sstream>

enum class Suffix
{
  B,
  W,
  L,
  Q,
};

inline std::ostream& operator<<(std::ostream& os, const Suffix& suf)
{
  switch (suf)
  {
    case Suffix::B:
      os << "b";
      break;
    case Suffix::W:
      os << "w";
      break;
    case Suffix::L:
      os << "l";
      break;
    case Suffix::Q:
      os << "q";
      break;
    default: assert(false);
  }
  return os;
}

namespace Reg
{
  enum class GPR
  {
    RAX, EAX, AX, AL,
    RBX, EBX, BX, BL,
    RCX, ECX, CX, CL,
    RDX, EDX, DX, DL,

    RDI, EDI, DI, DIL,
    RSI, ESI, SI, SIL,

    R8, R8D, R8W, R8B,
    R9, R9D, R9W, R9B,
    R10, R10D, R10W, R10B,
    R11, R11D, R11W, R11B,
    R15, R15D, R15W, R15B,

    RBP, EBP, BP, BPL,
    RSP, ESP, SP, SPL
  };

  enum class FPR
  {
    ZMM0, YMM0, XMM0,
    ZMM1, YMM1, XMM1,
    ZMM2, YMM2, XMM2,
    ZMM3, YMM3, XMM3,
    ZMM4, YMM4, XMM4,
    ZMM5, YMM5, XMM5,
    ZMM6, YMM6, XMM6,
    ZMM7, YMM7, XMM7,
    ZMM8, YMM8, XMM8,
    ZMM9, YMM9, XMM9,
    ZMM10, YMM10, XMM10,
    ZMM11, YMM11, XMM11,
    ZMM12, YMM12, XMM12,
    ZMM13, YMM13, XMM13,
    ZMM14, YMM14, XMM14,
    ZMM15, YMM15, XMM15
  };

  inline std::ostream& operator<<(std::ostream& os, const GPR& gpr);
  inline std::ostream& operator<<(std::ostream& os, const FPR& fpr);

  inline std::ostream& operator<<(std::ostream& os, const std::variant<Reg::GPR, Reg::FPR>& var)
  {
    std::visit([&](auto&& arg) { os << arg; }, var);
    return os;
  }

  inline bool is_fp(const std::variant<Reg::GPR, Reg::FPR>& var)
  {
    return std::holds_alternative<Reg::FPR>(var);
  }

  static GPR to64(GPR reg)
  {
    int id = static_cast<int>(reg);
    return static_cast<GPR>((id / 4) * 4);
  }

  static GPR to32(GPR reg)
  {
    int id = static_cast<int>(reg);
    return static_cast<GPR>((id / 4) * 4 + 1);
  }

  static GPR to16(GPR reg)
  {
    int id = static_cast<int>(reg);
    return static_cast<GPR>((id / 4) * 4 + 2);
  }

  static GPR to8(GPR reg)
  {
    int id = static_cast<int>(reg);
    return static_cast<GPR>((id / 4) * 4 + 3);
  }

  static FPR to512(FPR reg)
  {
    int id = static_cast<int>(reg);
    return static_cast<FPR>((id / 3) * 3);
  }

  static FPR to256(FPR reg)
  {
    int id = static_cast<int>(reg);
    return static_cast<FPR>((id / 3) * 3 + 1);
  }

  static FPR to128(FPR reg)
  {
    int id = static_cast<int>(reg);
    return static_cast<FPR>((id / 3) * 3 + 2);
  }

  static uint32_t tosz(GPR reg)
  {
    int id = static_cast<int>(reg) % 4;
    switch (id)
    {
      case (0): return 8;
      case (1): return 4;
      case (2): return 2;
      case (3): return 1;
      default: assert(false);
    }
  }

  static uint32_t tosz(FPR reg)
  {
    int id = static_cast<int>(reg) % 3;
    switch (id)
    {
      case (0): return 512;
      case (1): return 256;
      case (2): return 128;
      default: assert(false);
    }
  }

  static GPR as_sz(GPR reg, uint32_t bytes)
  {
    switch (bytes)
    {
      case (1): return to8(reg);
      case (2): return to16(reg);
      case (4): return to32(reg);
      case (8): return to64(reg);
      default: return to64(reg); assert(false && "Invalid reg size");
    }
  }

  static FPR as_sz(FPR reg, uint32_t bytes)
  {
    switch (bytes)
    {
      case (128): return to128(reg);
      case (256): return to256(reg);
      case (512): return to512(reg);
      default: return to128(reg); assert(false && "Invalid reg size");
    }
  }

  inline std::ostream& operator<<(std::ostream& os, const GPR& gpr)
  {
    switch (gpr)
    {
      case GPR::RAX: return os << "%rax";
      case GPR::EAX: return os << "%eax";
      case GPR::AX:  return os << "%ax";
      case GPR::AL:  return os << "%al";

      case GPR::RBX: return os << "%rbx";
      case GPR::EBX: return os << "%ebx";
      case GPR::BX:  return os << "%bx";
      case GPR::BL:  return os << "%bl";

      case GPR::RCX: return os << "%rcx";
      case GPR::ECX: return os << "%ecx";
      case GPR::CX:  return os << "%cx";
      case GPR::CL:  return os << "%cl";

      case GPR::RDX: return os << "%rdx";
      case GPR::EDX: return os << "%edx";
      case GPR::DX:  return os << "%dx";
      case GPR::DL:  return os << "%dl";

      case GPR::RDI: return os << "%rdi";
      case GPR::EDI: return os << "%edi";
      case GPR::DI:  return os << "%di";
      case GPR::DIL: return os << "%dil";

      case GPR::RSI: return os << "%rsi";
      case GPR::ESI: return os << "%esi";
      case GPR::SI:  return os << "%si";
      case GPR::SIL: return os << "%sil";

      case GPR::R8:  return os << "%r8";
      case GPR::R8D: return os << "%r8d";
      case GPR::R8W: return os << "%r8w";
      case GPR::R8B: return os << "%r8b";

      case GPR::R9:  return os << "%r9";
      case GPR::R9D: return os << "%r9d";
      case GPR::R9W: return os << "%r9w";
      case GPR::R9B: return os << "%r9b";

      case GPR::R10:  return os << "%r10";
      case GPR::R10D: return os << "%r10d";
      case GPR::R10W: return os << "%r10w";
      case GPR::R10B: return os << "%r10b";

      case GPR::R11:  return os << "%r11";
      case GPR::R11D: return os << "%r11d";
      case GPR::R11W: return os << "%r11w";
      case GPR::R11B: return os << "%r11b";

      case GPR::R15:  return os << "%r15";
      case GPR::R15D: return os << "%r15d";
      case GPR::R15W: return os << "%r15w";
      case GPR::R15B: return os << "%r15b";

      case GPR::RBP: return os << "%rbp";
      case GPR::RSP: return os << "%rsp";
      
      default: assert(false && "Unknown GPR register");
    }
  }

  inline std::ostream& operator<<(std::ostream& os, const FPR& fpr)
  {
    switch (fpr)
    {
      case FPR::ZMM0:  return os << "%zmm0";
      case FPR::YMM0:  return os << "%ymm0";
      case FPR::XMM0:  return os << "%xmm0";

      case FPR::ZMM1:  return os << "%zmm1";
      case FPR::YMM1:  return os << "%ymm1";
      case FPR::XMM1:  return os << "%xmm1";

      case FPR::ZMM2:  return os << "%zmm2";
      case FPR::YMM2:  return os << "%ymm2";
      case FPR::XMM2:  return os << "%xmm2";

      case FPR::ZMM3:  return os << "%zmm3";
      case FPR::YMM3:  return os << "%ymm3";
      case FPR::XMM3:  return os << "%xmm3";

      case FPR::ZMM4:  return os << "%zmm4";
      case FPR::YMM4:  return os << "%ymm4";
      case FPR::XMM4:  return os << "%xmm4";

      case FPR::ZMM5:  return os << "%zmm5";
      case FPR::YMM5:  return os << "%ymm5";
      case FPR::XMM5:  return os << "%xmm5";

      case FPR::ZMM6:  return os << "%zmm6";
      case FPR::YMM6:  return os << "%ymm6";
      case FPR::XMM6:  return os << "%xmm6";

      case FPR::ZMM7:  return os << "%zmm7";
      case FPR::YMM7:  return os << "%ymm7";
      case FPR::XMM7:  return os << "%xmm7";

      case FPR::ZMM8:  return os << "%zmm8";
      case FPR::YMM8:  return os << "%ymm8";
      case FPR::XMM8:  return os << "%xmm8";

      case FPR::ZMM9:  return os << "%zmm9";
      case FPR::YMM9:  return os << "%ymm9";
      case FPR::XMM9:  return os << "%xmm9";

      case FPR::ZMM10: return os << "%zmm10";
      case FPR::YMM10: return os << "%ymm10";
      case FPR::XMM10: return os << "%xmm10";

      case FPR::ZMM11: return os << "%zmm11";
      case FPR::YMM11: return os << "%ymm11";
      case FPR::XMM11: return os << "%xmm11";

      case FPR::ZMM12: return os << "%zmm12";
      case FPR::YMM12: return os << "%ymm12";
      case FPR::XMM12: return os << "%xmm12";

      case FPR::ZMM13: return os << "%zmm13";
      case FPR::YMM13: return os << "%ymm13";
      case FPR::XMM13: return os << "%xmm13";

      case FPR::ZMM14: return os << "%zmm14";
      case FPR::YMM14: return os << "%ymm14";
      case FPR::XMM14: return os << "%xmm14";

      case FPR::ZMM15: return os << "%zmm15";
      case FPR::YMM15: return os << "%ymm15";
      case FPR::XMM15: return os << "%xmm15";

      default: assert(false && "Unknown FPR register");
    }
  }
};

struct ASMOperand;

struct RegPool
{
  std::unordered_map<Reg::GPR, int> gpr_usage;
  std::unordered_map<Reg::FPR, int> fpr_usage;

  const std::vector<Reg::GPR> gpr_pool = {
    Reg::GPR::RAX, Reg::GPR::RCX, Reg::GPR::RDX,
    Reg::GPR::R8, Reg::GPR::R9, Reg::GPR::R10, Reg::GPR::R11
  };

  const std::vector<Reg::FPR> fpr_pool = {
    Reg::FPR::XMM8, Reg::FPR::XMM9, Reg::FPR::XMM10,
    Reg::FPR::XMM11, Reg::FPR::XMM12, Reg::FPR::XMM13, Reg::FPR::XMM14
  };

  explicit RegPool()
  {
    for (auto& reg : gpr_pool) gpr_usage[to64(reg)] = 0;
    for (auto& reg : fpr_pool) fpr_usage[to128(reg)] = 0;
  }

  Reg::GPR alloc_any_gpr(uint sz)
  {
    for (auto& reg : gpr_pool)
    {
      if (gpr_usage.at(reg) > 0) continue;
      gpr_usage.at(reg)++;
      switch (sz)
      {
        case (1): return to8(reg);
        case (2): return to16(reg);
        case (4): return to32(reg);
        case (8): return to64(reg);
        default:  return to64(reg); assert(false && "Unrecognized alloc size");
      }
    }
    assert(false && "Couldn't allocate GPR");
  }

  Reg::FPR alloc_any_fpr(uint sz)
  {
    for (auto& reg : fpr_pool)
    {
      if (fpr_usage.at(reg) > 0) continue;
      fpr_usage.at(reg)++;
      switch (sz)
      {
        case (16): return to128(reg);
        case (32): return to256(reg);
        case (64): return to512(reg);
        default:  return to128(reg); assert(false && "Unrecognized alloc size");
      }
    }
    assert(false && "Couldn't allocate FPR");
  }

  bool contains(Reg::GPR gpr)
  {
    return std::find(gpr_pool.begin(), gpr_pool.end(), gpr) != gpr_pool.end();
  }

  bool contains(Reg::FPR fpr)
  {
    return std::find(fpr_pool.begin(), fpr_pool.end(), fpr) != fpr_pool.end();
  }

  void lock(Reg::GPR reg)
  { gpr_usage.at(to64(reg))++; }

  void lock(Reg::FPR reg)
  { fpr_usage.at(to128(reg))++; }

  void unlock(Reg::GPR reg)
  {
    assert(gpr_usage.at(to64(reg)) > 0 && "lock/unlock mismatch");
    gpr_usage.at(to64(reg))--;
  }

  void unlock(Reg::FPR reg)
  {
    assert(fpr_usage.at(to128(reg)) > 0 && "lock/unlock mismatch");
    fpr_usage.at(to128(reg))--;
  }
};

class CodeGen;

struct PreOper
{
  RegPool* pool;
  std::variant<Reg::GPR, Reg::FPR> base_reg;
  bool indirect;
  int32_t offset;
  bool should_free;
  Type type;

  struct PairHash
  {
    template <class T, class U>
    uint32_t operator()(const std::pair<T, U>& p) const
    {
      auto h1 = std::hash<T>{}(p.first);
      auto h2 = std::hash<U>{}(p.second);
      return h1 ^ (h2 << 1);
    }
  };

  std::unordered_map<std::pair<int, int>, std::string, PairHash> m_ext_opc = {
    { { 1, 4 }, "movsbl" },
    { { 1, 8 }, "movsbq" },
    { { 2, 4 }, "movswl" },
    { { 4, 8 }, "movslq" },
  };

public:

  explicit PreOper(Type t, std::variant<Reg::GPR, Reg::FPR> r, bool ind, int32_t offs, bool sf, RegPool* p)
    : base_reg(r), indirect(ind), offset(offs), should_free(sf), pool(p), type(std::move(t))
  {
    if (indirect) assert(!std::holds_alternative<Reg::FPR>(r) && "Cannot have FPR as pointer");
  }

  PreOper(const PreOper& other) = delete;
  PreOper(PreOper&& other) noexcept
    :
    pool(other.pool), base_reg(other.base_reg), indirect(other.indirect),
    offset(other.offset), should_free(other.should_free), type(other.type)
  { other.should_free = false; }
  ~PreOper()
  {
    if (should_free)
      std::visit(
        Utils::overloaded
        {
          [this](Reg::GPR gpr) { pool->unlock(gpr); },
          [this](Reg::FPR fpr) { pool->unlock(fpr); }
        }
        ,base_reg
      );
  }

  [[nodiscard]] uint32_t size() const
  {
    if (indirect) return type.inner()->size();
    return type.size();
  }

  [[nodiscard]] std::variant<Reg::GPR, Reg::FPR> effective_reg() const
  {
    if (indirect) return to64(std::get<Reg::GPR>(base_reg));
    return std::visit(
      Utils::overloaded
      {
        [this](Reg::GPR gpr) -> std::variant<Reg::GPR, Reg::FPR> { return Reg::as_sz(gpr, type.size()); },
        [this](Reg::FPR fpr) -> std::variant<Reg::GPR, Reg::FPR> { return Reg::as_sz(fpr, type.size()); }
      }, base_reg
    );
  }

  friend std::ostream& operator<<(std::ostream& os, const PreOper& po)
  {
    auto print_reg = [&os](const auto& reg) { os << reg; };
    if (po.indirect)
    {
      if (po.offset != 0)
      {
        os << po.offset << "(";
        std::visit(print_reg, po.effective_reg());
        os << ")";
      }
      else
      {
        os << "(";
        std::visit(print_reg, po.effective_reg());
        os << ")";
      }
    }
    else std::visit(print_reg, po.effective_reg());
    return os;
  }

};

struct RegGuard
{
  RegPool& pool;
  Reg::GPR reg;

  RegGuard(RegPool& p, uint32_t sz) : pool(p), reg(p.alloc_any_gpr(sz)) {}
  RegGuard(RegPool& p, Reg::GPR gpr) : pool(p), reg(gpr) {}
  RegGuard(const RegGuard&) = delete;
  ~RegGuard() { pool.unlock(reg); }

  operator Reg::GPR() const { return reg; }
  RegGuard& operator=(const RegGuard&) = delete;
  friend std::ostream& operator<<(std::ostream& os, const RegGuard& g) { return os << g.reg; }
};

using OperandPtr = std::variant<PreOper*, IR::Operand*>;

class CodeGen
{
public:
  explicit CodeGen(std::vector<IR::Instruct>& instructs);
  std::string gen_code();

private:

  std::string format_operand(const IR::Operand& oper)
  {
    struct Formatter
    {
      CodeGen* gen;
      std::string operator()(const IR::Reg& r) const { return std::to_string(gen->reg_offset(r)) + "(%rbp)"; }
      std::string operator()(const IR::Lit& l) const
      {
        if (l.is_float())
        { assert(false); }
        else if (l.is_str()) assert(false && "NYI");
        else return "$" + std::to_string(l.as_int());
      }
      std::string operator()(const IR::Mem& m) const
      {
        std::stringstream res;
        if (m.disp)
          res << std::visit(*this, *m.disp);

        if (m.base || m.index || m.scale)
        {
          res << "(";
          if (m.base) res << std::to_string(gen->reg_offset(m.base.value())) + "(%rbp)";
          if (m.index || m.scale)
          {
            res << ", ";
            if (m.index) res << std::to_string(gen->reg_offset(m.index.value())) + "(%rbp)";
            if (m.scale) res << ", " << "$" + std::to_string(m.scale->as_int());
          }
          res << ")";
        }
        return res.str();

      }
      std::string operator()(const IR::Label& l) const
      { return l.name + ((l.kind == IR::Label::BSS) || (l.kind == IR::Label::RODATA) ? "(%rip)" : ""); }
    };
    return std::visit(Formatter{this}, oper);
  }
  void precomp_stack_size(const IR::Label& l, const std::vector<IR::Instruct>& inst);

  void gen_instr(IR::Instruct& instr);
  void gen_store(IR::Store& store);
  void gen_gstore(IR::GStore& gstore);
  void gen_binop(IR::Binop& binop);
  void gen_unop(IR::Unop& unop);
  void gen_jmp(IR::Jmp& jmp);
  void gen_jeq(IR::Jeq& jeq);
  void gen_jne(IR::Jne& jne);
  void gen_label(IR::Label& label);
  void gen_fn(IR::Func& fn);
  void gen_call(IR::Call& call);
  void gen_params(std::vector<IR::Param>& params);
  void gen_ret(IR::Ret& ret);
  void gen_alloca(IR::Alloca& alc);
  void gen_impc(IR::ImpCast& impc);
  void gen_nop(IR::Nop& nop);
  void cleanup_stack();
  static Suffix sfx(const ASMOperand& oper);
  static Suffix sfx(OperandPtr oper);
  static Suffix sfx(uint32_t sz);
  static uint32_t oper_sz(OperandPtr oper);
  int32_t reg_offset(const IR::Reg& reg);

  PreOper prepare_oper(OperandPtr oper);
  PreOper prepare_dest(OperandPtr oper);
  PreOper prepare_oper(IR::Operand* oper);
  PreOper prepare_dest(IR::Operand* oper);
  PreOper prepare_oper(PreOper* oper);
  PreOper prepare_dest(PreOper* oper);

  void MOV(OperandPtr lhs, OperandPtr rhs);
  void ASG(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void ADD(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void SUB(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void MUL(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void DIV(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void CMP(OperandPtr lhs, OperandPtr rhs);
  void RSH(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void LSH(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void OR(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void AND(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void XOR(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void LT(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void GT(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void LTE(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void GTE(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void EQ(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void NEQ(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void IDX(OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  void IMPC(OperandPtr dest, OperandPtr val, IR::ImpCast::CastOp op);
  void SETCC(const std::string& set_op, OperandPtr lhs, OperandPtr rhs, OperandPtr dest);

  void ADDR_OF(OperandPtr src, OperandPtr dest);
  void DEREF(OperandPtr src, OperandPtr dest);
  void INC(OperandPtr src, OperandPtr dest);
  void DEC(OperandPtr src, OperandPtr dest);
  void NOT(OperandPtr src, OperandPtr dest);
  void NEG(OperandPtr src, OperandPtr dest);

private:
  RegPool pool;
  int32_t m_rbpoff;
  std::unordered_map<IR::Reg, int32_t> m_reg_offset;
  std::unordered_map<std::string, uint32_t> m_ln_to_rsz;
  std::vector<IR::Instruct> m_instructs;
  std::stringstream m_bss;
  std::stringstream m_data;
  std::stringstream m_text;
  std::stringstream m_rodata;
};

struct ASMOperand : std::variant<IR::Operand, Reg::GPR>
{
  using std::variant<IR::Operand, Reg::GPR>::variant;
  friend std::ostream& operator<<(std::ostream& os, ASMOperand& asm_oper)
  {
    return os;
  }
};

#endif // CODEGEN_H
