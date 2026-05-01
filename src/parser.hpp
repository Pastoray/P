#ifndef PARSER_H
#define PARSER_H

#include <cstdint>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "tokenizer.hpp"
#include "shared_ops.hpp"
#include "type.hpp"
#include "utils.hpp"

class Parser;

namespace Node
{
  struct ExprVisitor;
  struct Expr
  {
    Expr() = default;
    virtual ~Expr() = default;
    virtual void accept(ExprVisitor& v) = 0;
    virtual void dump(int ident) const = 0;
  };
  struct UnExpr;
  struct BinExpr;
  struct Lit;
  struct Ident;
  struct Int;
  struct Call;
  struct ExprVisitor
  {
    virtual ~ExprVisitor() = default;
    virtual void visit(const UnExpr& un_expr) = 0;
    virtual void visit(const BinExpr& bin_expr) = 0;
    virtual void visit(const Ident& ident) = 0;
    virtual void visit(const Int& int_) = 0;
    virtual void visit(const Call& call) = 0;
  };
  struct Lit : Expr
  {
  };
  struct Asgn;
  struct ReAsgn;
  struct If;
  struct For;
  struct Continue;
  struct Break;
  struct Ret;
  struct ExprStmt;
  struct StmtVisitor
  {
    virtual ~StmtVisitor() = default;
    virtual void visit(const Asgn& asgn) = 0;
    // virtual void visit(const ReAsgn& asgn) = 0;
    virtual void visit(const If& if_) = 0;
    virtual void visit(const For& for_) = 0;
    virtual void visit(const Continue& cnt) = 0;
    virtual void visit(const Break& brk) = 0;
    virtual void visit(const Ret& ret) = 0;
    virtual void visit(const ExprStmt& expr_stmt) = 0;
  };
  struct Stmt
  {
    Stmt() = default;
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitor& v) = 0;
    virtual void dump(int ident) const = 0;
  };
  struct DeclVisitor;
  struct Decl
  {
    Decl() = default;
    virtual ~Decl() = default;
    virtual void accept(DeclVisitor& v) = 0;
    virtual void dump(int ident) const = 0;
  };
  struct Callable
  {
  };

  struct Int : Lit
  {
    explicit Int(const int val) : val(val) {}
    void accept(ExprVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::Int {\n";
      std::cout << std::string(ident + 1, '\t') << "val: " << val << '\n';
      std::cout << std::string(ident, '\t') << "}\n";
    }
    int val;
  };

  struct Ident : Lit
  {
    static uint32_t nid;
    uint32_t id;
    std::string name;

    explicit Ident(std::string name) : id(nid++), name(std::move(name)) {}
    // Ident(const Ident& other) = delete;
    // Ident& operator=(const Ident& other) = delete;
    Ident(const Ident& other) : id(nid++), name(other.name) {}
    Ident& operator=(const Ident& other)
    {
      if (this != &other) { name = other.name; }
      return *this;
    }

    Ident(Ident&& other) noexcept : id(other.id), name(std::move(other.name)) {}

    bool operator==(const Ident& other) const { return id == other.id; }
    
    void accept(ExprVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::Ident {\n";
      std::cout << std::string(ident + 1, '\t') << "name: " << name << '\n';
      std::cout << std::string(ident + 1, '\t') << "id: " << id << '\n';
      std::cout << std::string(ident, '\t') << "}\n";
    }
  };

  struct UnExpr : Expr
  {
    explicit UnExpr(std::shared_ptr<Expr> expr, UnOp op, bool pref) : expr(std::move(expr)), op(op), pref(pref) {};
    void accept(ExprVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::UnExpr {\n";
      if (expr)
      {
        std::cout << std::string(ident + 1, '\t') << "expr: " << '\n';
        expr->dump(ident + 1);
      }
      std::cout << std::string(ident + 1, '\t') << "op: " << op << '\n';
      std::cout << std::string(ident + 1, '\t') << "kind: " << (pref ? "prefix" : "suffix") << '\n';
      std::cout << std::string(ident, '\t') << "}\n";
    }
    std::shared_ptr<Expr> expr;
    UnOp op;
    bool pref;
  };

  struct BinExpr : Expr
  {
    explicit BinExpr(std::shared_ptr<Expr> lhs, BinOp op,
                     std::shared_ptr<Expr> rhs)
        : lhs(std::move(lhs)), op(op), rhs(std::move(rhs))
    {
    }

    void accept(ExprVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::BinExpr {\n";
      if (lhs)
      {
        std::cout << std::string(ident + 1, '\t') << "lhs: " << '\n';
        lhs->dump(ident + 1);
      }
      std::cout << std::string(ident + 1, '\t') << "op: " << op << '\n';
      if (rhs)
      {
        std::cout << std::string(ident + 1, '\t') << "rhs: " << '\n';
        rhs->dump(ident + 1);
      }
      std::cout << std::string(ident, '\t') << "}\n";
    }
    std::shared_ptr<Expr> lhs;
    BinOp op;
    std::shared_ptr<Expr> rhs;
  };

  struct Call : Expr
  {
    explicit Call(std::string callable) : callable(std::move(callable))
    {
    }
    void accept(ExprVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::Call {\n";
      std::cout << std::string(ident + 1, '\t') << "callable: " << callable.id << '\n';
      std::cout << std::string(ident, '\t') << "}\n";
    }

    Ident callable;
    std::vector<std::shared_ptr<Expr>> args;
  };

  struct Scope
  {
    std::vector<std::variant<std::shared_ptr<Expr>, std::shared_ptr<Stmt>,
                             std::shared_ptr<Decl>>> terms;

    void push_back(std::shared_ptr<Stmt> stmt)
    {
      terms.emplace_back(std::move(stmt));
    }

    void push_back(std::shared_ptr<Expr> expr)
    {
      terms.emplace_back(std::move(expr));
    }

    void push_back(std::shared_ptr<Decl> decl)
    {
      terms.emplace_back(std::move(decl));
    }
  };

  struct Func;
  struct Struct;
  struct DeclVisitor
  {
    virtual ~DeclVisitor() = default;
    virtual void visit(const Func& func) = 0;
    virtual void visit(const Struct& strct) = 0;
  };

  struct Param : Ident
  {
    explicit Param(Type type, std::string id) : Ident(std::move(id)), type(std::move(type)) {}
    Type type;
  };

  struct Func : Decl, Callable
  {
    explicit Func(Ident&& id, Type type) : id(std::move(id)), ret_type(std::move(type)) {}
    void accept(DeclVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::Func: " << id.name << " {\n";
      
      if (!params.empty()) {
        std::cout << std::string(ident + 1, '\t') << "params: [\n";
        for (const auto& param : params) {
          param.dump(ident + 2);
        }
        std::cout << std::string(ident + 1, '\t') << "]\n";
      }

      std::cout << std::string(ident + 1, '\t') << "returns: " << "Type_Info_Here" << "\n";

      if (scope.has_value()) {
        std::cout << std::string(ident + 1, '\t') << "body: {\n";
        for (const auto& term : scope->terms) {
          std::visit([ident](auto&& arg) {
            if (arg) arg->dump(ident + 2);
          }, term);
        }
        std::cout << std::string(ident + 1, '\t') << "}\n";
      } else {
        std::cout << std::string(ident + 1, '\t') << "body: <forward_declaration>\n";
      }

      std::cout << std::string(ident, '\t') << "}\n";
    }

    Ident id;
    std::vector<Param> params;
    std::optional<Scope> scope; // std::optional for forward declarations
    Type ret_type;
  };

  struct Struct : Decl
  {
    explicit Struct(Ident&& id) : id(std::move(id)) {}
    void accept(DeclVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override {
      std::cout << std::string(ident, '\t') << "Node::Struct: " << id.name << " {\n";
      for (const auto& [type, member_id] : members) {
          // Assuming Type has a way to be printed, or just print the name for now
          std::cout << std::string(ident + 1, '\t') << member_id.name << "\n";
      }
      std::cout << std::string(ident, '\t') << "}\n";
    }
    Ident id;
    std::vector<std::pair<Type, Ident>> members;
  };

  struct Import : Stmt
  {
    void accept(StmtVisitor& v) override {};
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::Import {\n";
      for (const auto& i : mod) { std::cout << std::string(ident + 1, '\t') << i.name << "\n"; }
      std::cout << std::string(ident, '\t') << "}\n";
    };
    std::vector<Ident> mod;
  };

  struct Asgn : Stmt
  {
    Ident id;
    std::shared_ptr<Type> type;
    std::optional<std::variant<std::shared_ptr<Expr>, std::shared_ptr<Decl>>> val;
    Asgn(std::shared_ptr<Type> type, std::string& id) : type(std::move(type)), id(id), val() {}
    Asgn(std::shared_ptr<Type> type, std::string& id, std::shared_ptr<Expr> expr)
        : type(std::move(type)), id(id), val(expr)
    {}
    void accept(StmtVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override {
      std::cout << std::string(ident, '\t') << "Node::Asgn {\n";
      std::cout << std::string(ident + 1, '\t') << "type: " << *type << "\n";
      std::cout << std::string(ident + 1, '\t') << "name: " << id.name << "\n";
      std::cout << std::string(ident + 1, '\t') << "id: " << id.id << "\n";
      
      // Dump the value (Expr or Decl)
      if (val)
        std::visit([ident](auto&& arg)
        {
          if (arg)
          {
            std::cout << std::string(ident + 1, '\t') << "val:\n";
            arg->dump(ident + 2);
          }
        }, *val);

      std::cout << std::string(ident, '\t') << "}\n";
    }
  };

  struct If : Stmt
  {
    If(std::shared_ptr<Expr> cond, Scope& scope, std::optional<If> if_)
        : cond(std::move(cond)),
          scope(std::make_shared<Scope>(std::move(scope))),
          elif (std::make_shared<std::optional<If>>(std::move(if_)))
    {
    }

    void accept(StmtVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::If {\n";
      
      if (cond)
      {
        std::cout << std::string(ident + 1, '\t') << "condition:\n";
        cond->dump(ident + 2);
      }

      if (scope)
      {
        std::cout << std::string(ident + 1, '\t') << "body (" << scope->terms.size() << " terms): {\n";
        for (const auto& term : scope->terms)
          std::visit([ident](auto&& arg) { if (arg) arg->dump(ident + 2); }, term);

        std::cout << std::string(ident + 1, '\t') << "}\n";
      }

      if (elif && *elif)
      {
        std::cout << std::string(ident + 1, '\t') << "elif/else:\n";
        (*elif)->dump(ident + 1);
      }

      std::cout << std::string(ident, '\t') << "}\n";
    }

    // else will just be and elif with cond set to True
    std::shared_ptr<Expr> cond;
    std::shared_ptr<Scope> scope;
    std::shared_ptr<std::optional<If>> elif;
  };

  struct For : Stmt
  {
    void accept(StmtVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::For {\n";
      if (init && *init) (*init)->dump(ident + 1);
      if (cond && *cond) (*cond)->dump(ident + 1);
      if (adv && *adv) (*adv)->dump(ident + 1);
      std::cout << std::string(ident + 1, '\t') << "scope...\n";
      std::cout << std::string(ident, '\t') << "}\n";
    }

    std::optional<std::shared_ptr<Asgn>> init;
    std::optional<std::shared_ptr<Expr>> cond;
    std::optional<std::shared_ptr<Expr>> adv;
    std::shared_ptr<Scope> scope;
  };

  struct Continue : Stmt
  {
    void accept(StmtVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override { std::cout << std::string(ident, '\t') << "continue\n"; }
  };

  struct Break : Stmt
  {
    void accept(StmtVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override { std::cout << std::string(ident, '\t') << "break\n"; }
  };

  struct ExprStmt : Stmt
  {
    explicit ExprStmt(std::shared_ptr<Expr> expr) : expr(std::move(expr)) {}
    void accept(StmtVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::ExprStmt {\n";
      if (expr) expr->dump(ident + 1);
      std::cout << std::string(ident, '\t') << "}\n";
    }

    std::shared_ptr<Expr> expr;
  };

  struct Ret : Stmt
  {
    explicit Ret(std::optional<std::shared_ptr<Expr>> expr)
        : ret_val(std::move(expr))
    {
    }
    void accept(StmtVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::Ret {\n";
      if (ret_val && *ret_val) (*ret_val)->dump(ident + 1);
      else std::cout << std::string(ident + 1, '\t') << "void\n";
      std::cout << std::string(ident, '\t') << "}\n";
    }
    std::optional<std::shared_ptr<Expr>> ret_val;
  };

  using Node = std::variant<std::shared_ptr<Stmt>, std::shared_ptr<Expr>,
                            std::shared_ptr<Decl>>;

}

class Parser
{
public:
  explicit Parser(std::vector<Token>& tokens);
  std::vector<Node::Node> parse_prog();
  std::optional<UnOp> parse_un_op();
  std::optional<BinOp> parse_bin_op();

  bool is_right_assoc(const BinOp& binop);
  bool is_right_assoc(const UnOp& unop);
  std::optional<std::shared_ptr<Node::Expr>> parse_paren_expr();
  std::optional<std::shared_ptr<Node::Expr>> parse_bin_expr(std::optional<std::shared_ptr<Node::Expr>> lhs, std::optional<BinOp> prev_op);
  std::optional<std::shared_ptr<Node::Expr>> parse_un_expr();
  std::optional<std::shared_ptr<Node::Expr>>
    parse_expr(std::optional<std::shared_ptr<Node::Expr>> lhs = std::nullopt, std::optional<BinOp> prev_op = std::nullopt);

  std::optional<Node::ExprStmt> parse_expr_stmt();
  std::optional<std::shared_ptr<Node::Stmt>> parse_stmt();
  std::optional<Node::Scope> parse_scope();
  std::optional<Node::Struct> parse_struct();
  std::optional<Node::Func> parse_func();

  std::optional<Node::Int> parse_lit_int();
  std::optional<Node::Ident> parse_lit_ident();
  std::optional<std::shared_ptr<Node::Lit>> parse_lit();
  std::optional<std::shared_ptr<Node::Decl>> parse_decl();
  std::optional<Node::If> parse_if_stmt();
  std::optional<Node::Asgn> parse_asgn_stmt();

  std::optional<Node::For> parse_for_stmt();
  std::optional<Node::Continue> parse_continue_stmt();
  std::optional<Node::Break> parse_break_stmt();
  std::optional<Node::Ret> parse_ret_stmt();
  std::optional<std::vector<Node::Param>> parse_param_list();
  std::optional<std::vector<std::shared_ptr<Node::Expr>>> parse_arg_list();
  std::optional<Node::Call> parse_call_expr();
  std::optional<Node::Node> parse_node();
  std::optional<std::shared_ptr<Type>> parse_type();
  void parse_arr(std::shared_ptr<Type>&);
  std::optional<Node::Import> parse_import();

private:
  [[nodiscard]] std::optional<Token> peek(int offset = 0);
  Token consume();

  template <typename T, typename = void>
  struct has_value_type : std::false_type
  {
  };

  template <typename T>
  struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type
  {
  };

  template <typename T>
  const static bool has_value_type_v = has_value_type<T>::value;

  template <typename T>
  std::enable_if_t<has_value_type_v<T>, typename T::value_type>
  strict(const T& opt)
  {
    if (opt.has_value())
      return opt.value();

    Utils::panic("expected value", '\n');
    return std::nullopt;
  }

private:
  const std::vector<Token> m_tokens;
  uint16_t m_index;
};

#endif // PARSER_H
