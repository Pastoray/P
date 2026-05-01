#ifndef IR_H
#define IR_H

#include "parser.hpp"
#include "sema.hpp"
#include "utils.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <stack>

namespace IR
{
  struct Lit
  {
    Type type;
    int32_t value;
    explicit Lit(const int32_t value, Type tp) : type(std::move(tp)), value(value) {}

    bool operator<(const Lit& other) const noexcept { return value < other.value; }
    bool operator==(const Lit& other) const noexcept { return value == other.value; }
    inline friend std::ostream& operator<<(std::ostream& os, const Lit& lit) { os << lit.value; return os; }
  };

  struct Reg
  {
    static uint32_t nid;
    Type type;
    uint32_t id;
    explicit Reg(Type tp) : type(std::move(tp)), id(nid++) {}

    bool operator<(const Reg& other) const noexcept { return id < other.id; }
    bool operator==(const Reg& rhs) const { return id == rhs.id; }
    inline friend std::ostream& operator<<(std::ostream& os, const Reg& reg)
    {
      os << "%" << reg.id << "[" << Type::to_string(reg.type) << "]";
      return os;
    }
  };

  struct Label
  {
    enum Kind { DATA, CODE };

    static uint32_t nid;
    Type type;
    uint32_t id;
    std::string name;
    Kind kind;

  private:
    explicit Label(std::string name, Kind k, Type tp) : type(std::move(tp)), id(nid++), name(std::move(name)), kind(k) {}

  public:
    static Label create_code(const std::optional<std::string>& str = {})
    {
      if (str) return Label(*str, CODE, Type(Type::Base::VOID));
      return Label("label_" + std::to_string(nid), CODE, Type(Type::Base::VOID));
    }

    static Label create_data(std::string name, Type tp)
    { return Label(std::move(name), DATA, std::move(tp)); }

    bool operator==(const Label& other) const noexcept { return name == other.name; }
    bool operator<(const Label& other) const noexcept { return name < other.name; }
    inline friend std::ostream& operator<<(std::ostream& os, const Label& label)
    {
      os << label.name << "[" << Type::to_string(label.type) << "]";
      return os;
    }
  };

  struct Mem
  {
    Type type;
    std::optional<std::variant<Lit, Label>> disp;
    std::optional<Reg> base;
    std::optional<Reg> index;
    std::optional<Lit> scale;

    explicit Mem(
      Type t,
      std::optional<std::variant<Lit, Label>> disp = std::nullopt,
      std::optional<Reg> base = std::nullopt,
      std::optional<Reg> index = std::nullopt,
      std::optional<Lit> scale = std::nullopt
    ) : type(std::move(t)), disp(std::move(disp)), base(std::move(base)), 
        index(std::move(index)), scale(std::move(scale)) {}

    inline bool operator<(const Mem& rhs)
    {
      const auto& lhs = *this;
      return std::tie(lhs.disp, lhs.base, lhs.index, lhs.scale) <
        std::tie(rhs.disp, rhs.base, rhs.index, rhs.scale);
    }

    inline bool operator==(const Mem& rhs) const
    {
      const auto& lhs = *this;
      return std::tie(lhs.disp, lhs.base, lhs.index, lhs.scale) ==
        std::tie(rhs.disp, rhs.base, rhs.index, rhs.scale);
    }

    inline friend std::ostream& operator<<(std::ostream& os, const Mem& mem)
    {
      if (mem.disp)
        std::visit([&os](auto&& arg) { os << arg; }, *mem.disp);

      if (mem.base || mem.index || mem.scale)
      {
        os << "(";
        if (mem.base) os << *mem.base;
        if (mem.index || mem.scale)
        {
          os << ", ";
          if (mem.index) os << *mem.index;
          if (mem.scale) os << ", " << *mem.scale;
        }
        os << ")";
      }
      return os;
    }
  };

  using Operand = std::variant<Lit, Reg, Mem, Label>;
  inline std::ostream& operator<<(std::ostream& os, const Operand& oper)
  {
    std::visit([&](auto&& arg) { os << arg; }, oper);
    return os;
  }

  inline const Type& get_type(const IR::Operand& op)
  {
    /*
    return std::visit(
      Utils::overloaded
      {
        [](const IR::Mem& mem) -> const Type& { return *mem.type.inner(); },
        [](auto&& arg) -> const Type& { return arg.type; }
      }, op
    );
    */
    return std::visit([](auto&& arg) -> const Type& { return arg.type; }, op);
  }
  
  struct Store
  {
    Operand dest;
    Operand val;

    explicit Store(Operand d, Operand v) : dest(std::move(d)), val(std::move(v))
    {
      assert(!std::holds_alternative<Lit>(dest) && "IR::Store->dest can't be literal");
    }

    inline friend std::ostream& operator<<(std::ostream& os, const Store& store)
    {
      /*
      if (auto reg = std::get_if<IR::Reg>(&store.dest))
        os << *reg << " = " << store.address;
      else if (auto str = std::get_if<Label>(&store.dest))
        os << *str << " = " << store.address;
      */
      os << store.dest << " = " << store.val;
      return os;
    }
  };


  struct GStore
  {
    Label dest;
    Operand val;

    explicit GStore(Label d, Operand v) : dest(std::move(d)), val(std::move(v)) {}

    inline friend std::ostream& operator<<(std::ostream& os, const GStore& store)
    {
      os << "G " << store.dest << " = " << store.val;
      return os;
    }
  };

  struct Binop
  {
    Operand dest;
    Operand lhs;
    Operand rhs;
    BinOp op;

    Binop(Operand dest, Operand lhs, BinOp op, Operand rhs)
        : dest(std::move(dest)), op(op), lhs(std::move(lhs)), rhs(std::move(rhs))
    {
    }

    inline friend std::ostream& operator<<(std::ostream& os, const Binop& b)
    {
      os << b.dest << " = ";
      os << b.lhs << " " << b.op << " " << b.rhs;
      return os;
    }
  };


  struct Unop
  {
    Operand dest;
    Operand src;
    UnOp op;

    inline friend std::ostream& operator<<(std::ostream& os, const Unop& unop)
    {
      os << unop.dest << " = " << unop.op << unop.src;
      return os;
    }
  };

  struct Jmp
  {
    Label target;
    explicit Jmp(Label t) : target(std::move(t)) {}
  };

  inline std::ostream& operator<<(std::ostream& os, const Jmp& jmp)
  {
    os << "JMP " << jmp.target;
    return os;
  }

  struct Jne
  {
    Operand lhs;
    Operand rhs;
    Label target;
  };

  inline std::ostream& operator<<(std::ostream& os, const Jne& jne)
  {
    os << jne.lhs << " != " << jne.rhs << " -> " << jne.target;
    return os;
  }

  struct Jeq
  {
    Operand lhs;
    Operand rhs;
    Label target;
  };

  inline std::ostream& operator<<(std::ostream& os, const Jeq& jeq)
  {
    os << jeq.lhs << " == " << jeq.rhs << " -> " << jeq.target;
    return os;
  }

  struct Param
  {
    Reg reg;
    explicit Param(Reg r) : reg(std::move(r)) {}

    inline friend std::ostream& operator<<(std::ostream& os, const Param& param)
    {
      os << "PARAM " << param.reg;
      return os;
    }
  };

  struct Call;
  struct Func;
  struct Ret;
  struct Nop;
  inline std::ostream& operator<<(std::ostream& os, const Nop& nop);

  struct Call
  {
    std::string callable;
    std::vector<Operand> args;
    Operand dest;
    explicit Call(Operand dest) : dest(std::move(dest)) {}
  };

  inline std::ostream& operator<<(std::ostream& os, const Call& call)
  {
    os << "CALL " << call.args.size() << ", ";
    for (auto& arg : call.args)
      os << arg << " ";
    os << ", RET IN = " << call.dest;

    return os;
  }

  struct Ret
  {
    Operand ret;
    explicit Ret(Operand ret) : ret(std::move(ret)) {}
  };

  inline std::ostream& operator<<(std::ostream& os, const Ret& ret)
  {
    os << "RET " << ret.ret;
    return os;
  }

  struct Lea
  {
    Operand lhs;
    Operand rhs;
  };

  struct Push
  {
    Operand oper;
  };

  struct ImpCast
  {
    enum CastOp
    {
      ZEXT,
      SEXT,
      TRUNC,
      SI2FP,
      UI2FP,
      FP2SI,
      FP2UI,
      FPEXT,
      FPTRUNC,
      BITCAST
    };

    inline friend std::ostream& operator<<(std::ostream& os, CastOp op)
    {
      switch (op)
      {
        case ZEXT:    return os << "ZEXT";
        case SEXT:    return os << "SEXT";
        case TRUNC:   return os << "TRUNC";
        case SI2FP:   return os << "SI2FP";
        case UI2FP:   return os << "UI2FP";
        case FP2SI:   return os << "FP2SI";
        case FP2UI:   return os << "FP2UI";
        case FPEXT:   return os << "FPEXT";
        case FPTRUNC: return os << "FPTRUNC";
        case BITCAST: return os << "BITCAST";
        default:      return os << "UNKNOWN";
      }
    }
    Operand dest;
    Operand val;
    CastOp op;
    explicit ImpCast(Operand d, Operand v, CastOp op) : dest(std::move(d)), val(std::move(v)), op(op) {}
    [[nodiscard]] static CastOp resolv_cast_op(const Type& frm, const Type& to)
    {
      if (frm.type == to.type)
        return CastOp::BITCAST;

      if (frm.size() == to.size() && frm.is_int() && to.is_int()) return CastOp::BITCAST;
      if (frm.is_int() && to.is_int())
      {
        if (frm.size() < to.size()) return (frm.is_signed() ? CastOp::SEXT : CastOp::ZEXT);
        else return CastOp::TRUNC;
      }
      if (frm.is_float() && to.is_int())
        return (to.is_signed() ? CastOp::FP2SI : CastOp::FP2UI);

      if (frm.is_ptr_t() && to.is_int()) return CastOp::ZEXT;
      if (frm.is_int() && to.is_ptr_t()) return CastOp::ZEXT;
      
      if (frm.is_int() && to.is_float())
        return (frm.is_signed() ? CastOp::SI2FP : CastOp::UI2FP);

      if (frm.size() == to.size()) return CastOp::BITCAST;
      return CastOp::BITCAST;
    }
    inline friend std::ostream& operator<<(std::ostream& os, const ImpCast& impc)
    {
      os << impc.dest << " = " << "[IMP_CAST(" << impc.op << ")] " << impc.val;
      return os;
    }
  };

  struct Alloca
  {
    Operand dest;
    std::vector<Operand> dims;

    explicit Alloca(Operand dest, std::vector<Operand> dims) : dest(std::move(dest)), dims(std::move(dims)) {}
    inline friend std::ostream& operator<<(std::ostream& os, const Alloca& alloc)
    {
      os << "[ALLOCA] ";
      os << alloc.dest << " = { ";
      for (auto& d : alloc.dims)
        os << d << " ";
      os << "}";
      return os;
    }
  };

  struct Nop
  {
    inline friend std::ostream& operator<<(std::ostream& os, const Nop& nop)
    {
      os << "NOP";
      return os;
    }
  };

  using Instruct = std::variant<GStore, Store, Binop, Unop, Jmp, Jeq, Jne, Label, Call, std::shared_ptr<Func>, ImpCast, Alloca, Ret, Nop>;
  inline std::ostream& operator<<(std::ostream& os, const Instruct& instr)
  {
    std::visit([&os](const auto& x) { os << x; }, instr);
    return os;
  }

  struct Func
  {
    explicit Func(Label label) : label(std::move(label)) {}
    Label label;
    std::vector<Param> params;
    std::vector<Instruct> body;

    inline friend std::ostream& operator<<(std::ostream& os, const Func& fn)
    {
      os << "FUNC " << fn.label << "\n";
      for (auto& param : fn.params)
        os << "PARAM: " << param << ", ";
      os << "\n";

      for (auto& instr : fn.body)
        std::visit([&os](auto&& arg) { os << arg << "\n"; }, instr);

      return os;
    }
  };

  inline std::ostream& operator<<(std::ostream& os, const std::shared_ptr<Func>& fn)
  {
    os << *fn;
    return os;
  }

}

template <>
struct std::hash<IR::Reg>
{
  size_t operator()(const IR::Reg& r) const noexcept
  { return std::hash<uint32_t>{}(r.id); }
};

class IRGen;

class IRGen
{
  struct Visitor : Node::DeclVisitor, Node::ExprVisitor, Node::StmtVisitor
  {
    IRGen& m_gen;
    explicit Visitor(IRGen& gen) : m_gen(gen) {}

    void visit(const Node::Func&) override;
    void visit(const Node::Struct&) override;

    void visit(const Node::UnExpr&) override;
    void visit(const Node::BinExpr&) override;
    void visit(const Node::Ident&) override;
    void visit(const Node::Int&) override;
    void visit(const Node::Call&) override;

    void visit(const Node::Asgn&) override;
    void visit(const Node::If&) override;
    void visit(const Node::For&) override;
    void visit(const Node::Continue&) override;
    void visit(const Node::Break&) override;
    void visit(const Node::Ret&) override;
    void visit(const Node::ExprStmt&) override;
  };

public:
  explicit IRGen(std::vector<Node::Node>& node, Sema::Analysis& anl);
  std::vector<IR::Instruct> gen();

private:
  void gen_fn(const Node::Func&);
  void gen_struct(const Node::Struct&);
  void gen_un_expr(const Node::UnExpr&);
  void gen_bin_expr(const Node::BinExpr&);
  void gen_ident(const Node::Ident&);
  void gen_int(const Node::Int&);
  void gen_asgn(const Node::Asgn&);

  void gen_if(const Node::If&);
  void gen_for(const Node::For&);
  void gen_continue(const Node::Continue&);
  void gen_break(const Node::Break&);
  void gen_ret(const Node::Ret&);
  void gen_call(const Node::Call&);
  void gen_scope(const Node::Scope&);
  void gen_expr_stmt(const Node::ExprStmt&);

private:
  void match_types();
  void coerce(const IR::Operand& oper, const Type& to);

private:
  const std::vector<Node::Node> m_nodes;
  const Sema::Analysis m_anl;
  Visitor m_visitor;
  uint32_t m_index;
  std::unordered_map<uint32_t, IR::Reg> aid_to_r;
  std::unordered_map<uint32_t, IR::Label> aid_to_l;
  std::unordered_map<IR::Reg, std::vector<IR::Operand>> r_to_arr_dims;

  std::stack<IR::Operand> m_stack;
  struct ScopeCtx
  {
    enum class Kind
    {
      LOOP,
      IF,
      FN
    };
    IR::Label start;
    IR::Label end;
    Kind kind;
  };
  std::vector<ScopeCtx> m_ctx_stack;

  std::vector<IR::Instruct>* m_instructs;
};
#endif // IR_H
