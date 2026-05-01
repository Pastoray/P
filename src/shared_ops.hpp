#include "logger.hpp"

#define OP(name, prec, op, len)
#define BIN_PURE_OPS \
  OP(ADD,     50, +, 1)  \
  OP(SUB,     50, -, 1)  \
  OP(MUL,     60, *, 1)  \
  OP(DIV,     60, /, 1)  \
  OP(LT,      30, <, 1)  \
  OP(GT,      30, >, 1)  \
  OP(IDX,     100, [], 2) \
  OP(LTE,     30, <=, 2) \
  OP(GTE,     30, >=, 2) \
  OP(EQ,      20, ==, 2) \
  OP(NEQ,     20, !=, 2) \
  OP(RSH,     40, >>, 2) \
  OP(LSH,     40, <<, 2) \
  OP(OR,      40, ||, 2) \
  OP(AND,     40, &&, 2) \
  OP(XOR,     40, ^, 1) 

#define BIN_WB_OPS  \
  OP(ASG,     10, =, 1)     \
  OP(ASG_ADD, 10, +=, 2)    \
  OP(ASG_SUB, 10, -=, 2)    \
  OP(ASG_MUL, 10, *=, 2)    \
  OP(ASG_DIV, 10, /=, 2)

#define BIN_OPS \
  BIN_PURE_OPS  \
  BIN_WB_OPS

#define UN_WB_OPS \
  OP(INC,     90, ++, 2)  \
  OP(DEC,     90, --, 2)

#define UN_PURE_OPS \
  OP(NOT,     90, !, 1)     \
  OP(NEG,     90, -, 1)

#define UN_PTR_OPS    \
  OP(ADDR_OF, 90, &, 1)  \
  OP(DEREF,   90, *, 1)

#define UN_OPS \
  UN_WB_OPS    \
  UN_PURE_OPS  \
  UN_PTR_OPS

#define ALL_OPS \
  BIN_OPS       \
  UN_OPS

#undef OP


enum class BinOp
{
  #define OP(name, prec, op, len) name,
  BIN_OPS
  #undef OP
};

enum class UnOp
{
  #define OP(name, prec, op, len) name,
  UN_OPS
  #undef OP
};

inline std::ostream& operator<<(std::ostream& os, const UnOp& unop)
{
  switch (unop)
  {
    #define OP(name, prec, op, len) case UnOp::name: os << #op; break;
    UN_OPS
    #undef OP
    default:
      Logger::warn("???");
  }
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const BinOp& binop)
{
  switch (binop)
  {
    #define OP(name, prec, op, len) case BinOp::name: os << #op; break;
    BIN_OPS
    #undef OP
    default:
      Logger::warn("???");
  }
  return os;
}

inline static int precedence(const BinOp& op)
{
  switch (op)
  {
    #define OP(name, prec, op, len) case BinOp::name: return prec;
    BIN_OPS
    #undef OP
    default:
      Logger::warn("Unknown operator"); return 0;
  }
}

inline static int precedence(const UnOp& op)
{
  switch (op)
  {
    #define OP(name, prec, op, len) case UnOp::name: return prec;
    UN_OPS
    #undef OP
    default:
      Logger::warn("Unknown operator"); return 0;
  }
}



