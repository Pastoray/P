#ifndef CODEGEN_H
#define CODEGEN_H
#include "ir.hpp"
#include "utils.hpp"

#include <cassert>
#include <cstdint>
#include <vector>
#include <sstream>
#include <queue>

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

  static GPR as_sz(GPR reg, uint32_t bytes)
  {
    switch (bytes)
    {
      case (1): return to8(reg);
      case (2): return to16(reg);
      case (4): return to32(reg);
      case (8): return to64(reg);
      default: assert(false && "Invalid reg size");
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
      
      default: assert(false);
    }
  }
};

struct ASMOperand;

struct RegPool
{
  /*
  std::queue<Reg::GPR> available;
  explicit RegPool() : available({ Reg::GPR::RAX, Reg::GPR::RDX, Reg::GPR::RCX, Reg::GPR::R8, Reg::GPR::R9 }) {}
  */
  std::unordered_map<Reg::GPR, int> usage;
  const std::vector<Reg::GPR> pool = { 
    Reg::GPR::RAX, Reg::GPR::RCX, Reg::GPR::RDX, 
    Reg::GPR::R8,  Reg::GPR::R9,  Reg::GPR::R10, Reg::GPR::R11 
  };

  explicit RegPool()
  { for (auto& reg : pool) usage[to64(reg)] = 0; }

  Reg::GPR alloc_any(uint sz)
  {
    for (auto& reg : pool)
    {
      if (usage.at(reg) > 0) continue;
      usage.at(reg)++;
      switch (sz)
      {
        case (1): return to8(reg);
        case (2): return to16(reg);
        case (4): return to32(reg);
        case (8): return to64(reg);
        default: assert(false && "Unrecognized alloc size");
      }
    }
    assert(false && "Couldn't allocate register");
  }

  void lock(Reg::GPR reg)
  { usage.at(to64(reg))++; }

  void unlock(Reg::GPR reg)
  { assert(usage.at(to64(reg)) > 0 && "lock/unlock mismatch"); usage.at(to64(reg))--; }

  /*
  Reg::GPR alloc(uint16_t sz)
  {
    assert(!available.empty());
    auto reg = available.front();
    available.pop();
    return as_sz(reg, sz);
  }

  Reg::GPR alloc(Reg::GPR gpr)
  {
    for (int i = 0; i < available.size(); i++)
    {
      auto top = available.front();
      available.pop();
      if (gpr == to32(top) || gpr == to64(top))
        return gpr;
      available.push(top);
    }
    assert(false && "Unavailable register...");
  }

  void free(Reg::GPR reg)
  { available.push(to64(reg)); }
  */
};

class CodeGen;

struct PreOper
{
  RegPool* pool;
  Reg::GPR base_reg;
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

  explicit PreOper(Type t, Reg::GPR r, bool ind, int32_t offs, bool sf, RegPool* p)
    : base_reg(r), indirect(ind), offset(offs), should_free(sf), pool(p), type(std::move(t)) {}

  PreOper(const PreOper& other) = delete;
  PreOper(PreOper&& other) noexcept
    :
    pool(other.pool), base_reg(other.base_reg), indirect(other.indirect),
    offset(other.offset), should_free(other.should_free), type(other.type)
  { other.should_free = false; }
  ~PreOper()
  { if (should_free) pool->unlock(base_reg); }

  [[nodiscard]] uint32_t size() const
  {
    if (indirect) return type.inner()->size();
    return type.size();
  }

  [[nodiscard]] Reg::GPR effective_reg() const
  {
    if (indirect) return to64(base_reg);
    return Reg::as_sz(base_reg, type.size());
  }

  void resize(const Type& new_t, std::stringstream& m_text)
  {
    m_text << "# resize (start)\n\t";
    if (indirect)
    {
      type = new_t;
      return;
      assert(false && "Memory read/write changing size...");
    }

    uint32_t old_sz = type.size();
    uint32_t new_sz = new_t.size();

    if (new_sz == old_sz)
    {
      type = new_t;
      m_text << "# resize (end) [skip]\n\t";
      return;
    }

    if (new_sz > old_sz)
    {
      std::pair<int, int> p = { old_sz, new_sz };
      if (m_ext_opc.find(p) == m_ext_opc.end())
        assert(false && "Conversion not allowed...");

      Reg::GPR src = as_sz(base_reg, old_sz);
      Reg::GPR dst = as_sz(base_reg, new_sz);
      m_text << m_ext_opc[p] << " " << src << ", " << dst << "\n\t";
    }
    else
    {
      m_text << "# resize (downcast)\n\t";
      /*
      if (new_sz == 4) reg = RegPool::to_32(reg);
      else if (new_sz == 8) reg = RegPool::to_64(reg);
      else assert(false);
      */
    }
    // osize = new_sz;
    type = new_t;
    m_text << "# resize (end)\n\t";
  }

  friend std::ostream& operator<<(std::ostream& os, const PreOper& po)
  {
    if (po.indirect)
    {
      if (po.offset != 0) os << po.offset << "(" << po.effective_reg() << ")";
      else os << "(" << po.effective_reg() << ")";
    }
    else os << po.effective_reg();
    return os;
  }

};

struct RegGuard
{
  RegPool& pool;
  Reg::GPR reg;

  RegGuard(RegPool& p, uint32_t sz) : pool(p), reg(p.alloc_any(sz)) {}
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

  std::string format_operand(IR::Operand& oper)
  {
    return std::visit(
      Utils::overloaded
      {
        [this](IR::Reg& r) { return std::to_string(reg_offset(r)) + "(%rbp)"; },
        [this](IR::Lit& l) { return "$" + std::to_string(l.value); },
        [this](IR::Mem& m)
        {
          std::stringstream res;
          if (m.disp)
            std::visit(
              Utils::overloaded
              {
                [&res](const IR::Lit& l)   { res << "$" + std::to_string(l.value); },
                [&res](const IR::Label& l) { res << l.name; }
              }, m.disp.value()
            );

          if (m.base || m.index || m.scale)
          {
            res << "(";
            if (m.base) res << std::to_string(reg_offset(m.base.value())) + "(%rbp)";
            if (m.index || m.scale)
            {
              res << ", ";
              if (m.index) res << std::to_string(reg_offset(m.index.value())) + "(%rbp)";
              if (m.scale) res << ", " << "$" + std::to_string(m.scale.value().value);
            }
            res << ")";
          }
          return res.str();
        },
        [this](IR::Label& l) { return l.name; }
      }, oper
    );
  }
  std::string format_operand(IR::Lit& l)
  {
    return "$" + std::to_string(l.value); 
  }
  std::string format_operand(IR::Reg& r)
  {
    return std::to_string(reg_offset(r)) + "(%rbp)"; 
  }

  std::string format_operand(IR::Mem& m)
  {
    std::stringstream res;
    if (m.disp)
      std::visit(
        Utils::overloaded
        {
          [&res](const IR::Lit& l)   { res << "$" + std::to_string(l.value); },
          [&res](const IR::Label& l) { res << l.name; }
        }, m.disp.value()
      );

    if (m.base || m.index || m.scale)
    {
      res << "(";
      if (m.base) res << std::to_string(reg_offset(m.base.value())) + "(%rbp)";
      if (m.index || m.scale)
      {
        res << ", ";
        if (m.index) res << std::to_string(reg_offset(m.index.value())) + "(%rbp)";
        if (m.scale) res << ", " << "$" + std::to_string(m.scale.value().value);
      }
      res << ")";
    }
    return res.str();
  }

  std::string format_operand(IR::Label& l)
  {
    return l.name; 
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
  void gen_param(IR::Param& param);
  void gen_params(std::vector<IR::Param>& params);
  void gen_ret(IR::Ret& ret);
  void gen_alloca(IR::Alloca& alc);
  void gen_impc(IR::ImpCast& impc);
  void gen_nop(IR::Nop& nop);
  void cleanup_stack();
  static Suffix sfx(const ASMOperand& oper);
  static Suffix sfx(OperandPtr oper);
  static Suffix sfx(uint32_t sz);
  // static Suffix get_com_sfx(OperandPtr oper1, OperandPtr oper2);
  // void promote(PreOper& oper1, PreOper& oper2);
  // uint32_t coerce_common(PreOper&, PreOper&);
  static uint32_t oper_sz(OperandPtr oper);
  int32_t reg_offset(IR::Reg& reg);
  // PreOper prepare(const IR::Operand& oper);
  // PreOper prepare(PreOper&& oper);

  PreOper prepare_oper(OperandPtr oper);
  PreOper prepare_dest(OperandPtr oper);
  PreOper prepare_oper(IR::Operand* oper);
  PreOper prepare_dest(IR::Operand* oper);
  PreOper prepare_oper(PreOper* oper);
  PreOper prepare_dest(PreOper* oper);
  // GPR prepare(OperandPtr oper);
  // GPR prepare(const GPR& oper);

  // void MOV(OperandPtr lhs, OperandPtr rhs);
  /*
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
  void emit_setcc(const std::string& set_op, OperandPtr lhs, OperandPtr rhs, OperandPtr dest);
  */

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

private:
  RegPool pool;
  int32_t m_rbpoff;
  std::unordered_map<IR::Reg, int32_t> m_reg_offset;
  std::unordered_map<std::string, uint32_t> m_ln_to_rsz;
  std::vector<IR::Instruct> m_instructs;
  std::stringstream m_bss;
  std::stringstream m_data;
  std::stringstream m_text;
};

struct ASMOperand : std::variant<IR::Operand, Reg::GPR>
{
  using std::variant<IR::Operand, Reg::GPR>::variant;
  friend std::ostream& operator<<(std::ostream& os, ASMOperand& asm_oper)
  {
    /*
    std::visit(
      [&os](auto&& arg)
      {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, IR::Operand>)
          os << CodeGen::format_operand(arg);
        else
          os << arg;
      }, asm_oper
    );
    */
    return os;
  }
};

#endif // CODEGEN_H
