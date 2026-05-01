#ifndef IROPTIMIZER_H
#define IROPTIMIZER_H
/*
#include "ir.hpp"

class IROptimizer
{
  public:
  explicit IROptimizer(std::vector<IR::Instruct>& instructs);

  void optimize();
  int32_t eval_comptime_expr(IR::Lit& lhs, BinOp op, IR::Lit& rhs);
  void constant_fold();
  void dead_code_eliminate();

  private:
  std::vector<IR::Instruct>& m_instructs;

  struct VarHash
  {
    size_t operator()(const IR::Operand& var) const
    {
      return std::visit([&](const auto& arg) -> size_t
        {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, IR::Reg>)
            return std::hash<uint16_t>{}(arg.id);
          else if constexpr (std::is_same_v<T, IR::Label>)
            return std::hash<std::string>{}(arg.name);
          return 0;
        },
      var);
    }
  };

  struct VarEqual
  {
    bool operator()(
      // const std::variant<IR::Reg, IR::Label>& a,
      // const std::variant<IR::Reg, IR::Label>& b
        const IR::Operand& a,
        const IR::Operand& b
      ) const
    {
      return a == b;
    }
  };
  std::unordered_map<IR::Operand, IR::Lit, VarHash, VarEqual> m_const_vals;
  uint16_t m_index;
};
*/

#endif // IROPTIMIZER_H
