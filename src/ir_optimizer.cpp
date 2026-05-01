#include "ir_optimizer.hpp"
/*
#include <unordered_set>

IROptimizer::IROptimizer(std::vector<IR::Instruct>& instructs)
    : m_instructs(instructs), m_index(0), m_const_vals() {}

void IROptimizer::optimize()
{
  constant_fold();
  dead_code_eliminate();
}

void IROptimizer::constant_fold()
{
  uint32_t i = 0;
  while (i < m_instructs.size())
  {
    auto& instruct = m_instructs[i];
    std::visit([this, &instruct](auto& arg)
    {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, IR::Store>)
      {
        if (auto reg = std::get_if<IR::Reg>(&arg.address))
        {
          if (auto it = m_const_vals.find(*reg); it != m_const_vals.end())
            m_const_vals.insert_or_assign(arg.dest, it->second);
        }
        else if (auto lbl = std::get_if<IR::Label>(&arg.address))
        {
          if (auto it = m_const_vals.find(*lbl); it != m_const_vals.end())
            m_const_vals.insert_or_assign(arg.dest, it->second);
        }
        else if (auto lit = std::get_if<IR::Lit>(&arg.address))
        {
          m_const_vals.insert_or_assign(arg.dest, *lit);
        }
      }
      else if constexpr (std::is_same_v<T, IR::Binop>)
      {
        if (auto lhs = std::get_if<IR::Reg>(&arg.lhs))
        {
          if (auto it = m_const_vals.find(*lhs); it != m_const_vals.end())
          {
            arg.lhs = it->second;
          }
        }
        else if (auto lhs = std::get_if<IR::Label>(&arg.lhs))
        {
          if (auto it = m_const_vals.find(*lhs); it != m_const_vals.end())
          {
            arg.lhs = it->second;
          }
        }
        if (auto rhs = std::get_if<IR::Reg>(&arg.rhs))
        {
          if (auto it = m_const_vals.find(*rhs); it != m_const_vals.end())
          {
            arg.rhs = it->second;
          }
        }
        else if (auto rhs = std::get_if<IR::Label>(&arg.lhs))
        {
          if (auto it = m_const_vals.find(*rhs); it != m_const_vals.end())
          {
            arg.rhs = it->second;
          }
        }
        auto* lhs_lit = std::get_if<IR::Lit>(&arg.lhs);
        auto* rhs_lit = std::get_if<IR::Lit>(&arg.rhs);
        if (lhs_lit && rhs_lit)
        {
          auto res = eval_comptime_expr(*lhs_lit, arg.op, *rhs_lit);
          m_const_vals.insert_or_assign(arg.dest, IR::Lit(res, 0));
          instruct = IR::Store{arg.dest, IR::Lit(res, 0)};
        }
      }
    }, instruct);
    i++;
  }
}

void IROptimizer::dead_code_eliminate()
{
  std::unordered_set<IR::Reg> used;
  uint32_t i = m_instructs.size();
  while (i--)
  {
    auto& instruct = m_instructs[i];
    std::visit([this, &used, &instruct](auto& arg)
    {
      if (auto expr = std::get_if<IR::Binop>(&arg.address))
      {
        if (auto reg = std::get_if<IR::Reg>(&expr.lhs))
        {
          used.insert(*reg);
        }
        if (auto reg = std::get_if<IR::Reg>(&expr.rhs))
        {
          used.insert(*reg);
        }
      }
      if (auto reg = std::get_if<IR::Reg>(&arg.address))
      {
        used.insert(*reg);
      }
      if (used.find(arg.dest) == used.end())
      {
        instruct = IR::Noop();
      }
    }, instruct);
  }
}

int32_t IROptimizer::eval_comptime_expr(IR::Lit& lhs, BinOp op, IR::Lit& rhs) {
  switch (op) {
    case BinOp::ADD: return lhs.value + rhs.value;
    case BinOp::SUB: return lhs.value - rhs.value;
    case BinOp::MUL: return lhs.value * rhs.value;
    case BinOp::DIV:
      if (rhs.value == 0)
      {
        Utils::panic("Math Error: Division by zero");
        return 0;
      }
      return lhs.value / rhs.value;
    case BinOp::LSH:
      return lhs.value << rhs.value;
    case BinOp::RSH:
      return lhs.value >> rhs.value;
    default:
      Utils::panic("Failed to evaluate compile time expression; unsupported operation");
      return 0;
  }
}
*/
