#ifndef PARSER_H
#define PARSER_H

#include <cstdint>
#include <iomanip>
#include <memory>
#include <optional>
#include <unordered_set>
#include <variant>
#include <vector>

#include "tokenizer.hpp"
#include "shared_ops.hpp"
#include "type.hpp"
#include "utils.hpp"

struct ScopeTracker
{
  struct LightScope
  {
    std::unordered_map<std::string, std::shared_ptr<Type>> types;
  };
  std::vector<LightScope> scopes;
  ScopeTracker() { scopes.emplace_back(); }

  void push_scope()
  {
    scopes.emplace_back();
  }

  void pop_scope()
  {
    scopes.pop_back();
  }

  void push_type(const std::string& name, std::shared_ptr<Type>& type)
  {
    scopes.back().types.try_emplace(name, type);
  }

  bool is_type(const std::string& name)
  {
    if (auto bt = Type::string_to_base_t(name)) return true;
    for (size_t i = scopes.size(); i > 0; i--)
      if (scopes[i - 1].types.count(name)) return true;
    return false;
  }

  std::shared_ptr<Type> get_type(const std::string& name)
  {
    if (auto bt = Type::string_to_base_t(name)) return std::make_shared<Type>(bt.value());
    for (size_t i = scopes.size(); i > 0; i--)
      if (scopes[i - 1].types.count(name)) return scopes[i - 1].types.at(name);
    return nullptr;
  }
};

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
  struct Double;
  struct String;
  struct Char;
  struct Call;
  struct Path;
  struct ExprVisitor
  {
    virtual ~ExprVisitor() = default;
    virtual void visit(const UnExpr& un_expr) = 0;
    virtual void visit(const BinExpr& bin_expr) = 0;
    virtual void visit(const Ident& ident) = 0;
    virtual void visit(const Path& path) = 0;
    virtual void visit(const Int& int_) = 0;
    virtual void visit(const Double& double_) = 0;
    virtual void visit(const String& string) = 0;
    virtual void visit(const Char& char_) = 0;
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
  struct TypeRef;
  struct TypeDef;
  struct StmtVisitor
  {
    virtual ~StmtVisitor() = default;
    virtual void visit(const TypeRef& ref) = 0;
    virtual void visit(const TypeDef& def) = 0;
    virtual void visit(const Asgn& asgn) = 0;
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

  struct Double : Lit
  {
    explicit Double(const double val) : val(val) {}
    void accept(ExprVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      auto old_prec = std::cout.precision();
      std::cout << std::string(ident, '\t') << "Node::Double {\n";
      std::cout << std::string(ident + 1, '\t') << "val: " << std::setprecision(20) << val << '\n';
      std::cout << std::string(ident, '\t') << "}\n";
      std::cout.precision(old_prec);
    }
    double val;
  };
  
  struct String : Lit
  {
    explicit String(std::string val) : val(std::move(val)) {}
    void accept(ExprVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::String {\n";
      std::cout << std::string(ident + 1, '\t') << "val: " << val << '\n';
      std::cout << std::string(ident, '\t') << "}\n";
    }
    std::string val;
  };

  struct Char : Lit
  {
    explicit Char(std::string val) : val(std::move(val)) {}
    void accept(ExprVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::Char {\n";
      std::cout << std::string(ident + 1, '\t') << "val: " << val << '\n';
      std::cout << std::string(ident, '\t') << "}\n";
    }
    std::string val;
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

  struct Path : Lit
  {
    explicit Path(std::vector<Ident> p) : path(std::move(p)) {}
    void accept(ExprVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::Path {\n";
      std::cout << std::string(ident + 1, '\t') << "path: ";
      for (size_t i = 0; i < path.size(); i++)
        std::cout << path[i].name << (i == path.size() - 1 ? std::string("") : std::string("::"));
      std::cout << '\n';
      std::cout << std::string(ident, '\t') << "}\n";
    }
    std::vector<Ident> path;
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
    explicit Call(std::shared_ptr<Expr> callable) : callable(std::move(callable)) {}
    explicit Call(std::shared_ptr<Expr> callable, std::vector<std::shared_ptr<Expr>> args)
      : callable(std::move(callable)), args(std::move(args)) {}

    void accept(ExprVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::Call {\n";
      std::cout << std::string(ident + 1, '\t') << "callable: " << '\n';
      callable->dump(ident + 1);
      std::cout << std::string(ident, '\t') << "}\n";
    }

    std::shared_ptr<Expr> callable;
    std::vector<std::shared_ptr<Expr>> args;
  };

  struct Scope
  {
    std::vector<std::variant<
      std::shared_ptr<Expr>,
      std::shared_ptr<Stmt>,
      std::shared_ptr<Decl>
    >> terms;

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

  struct TypeRef : Stmt
  {
    explicit TypeRef() : res_t(nullptr) {}
    explicit TypeRef(std::shared_ptr<Type> type) : res_t(std::move(type)) {}
    std::shared_ptr<Type> res_t;

    void accept(StmtVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override {}
  };

  struct Struct;
  struct Union;
  struct Enum;
  struct TypeDef : Stmt
  {
    explicit TypeDef(
      std::variant<
        std::shared_ptr<Struct>,
        std::shared_ptr<Union>,
        std::shared_ptr<Enum>
      > type, std::shared_ptr<Type> res_t
    ) : type(std::move(type)), res_t(std::move(res_t)) {}

    std::variant<
      std::shared_ptr<Struct>,
      std::shared_ptr<Union>,
      std::shared_ptr<Enum>
    > type;
    std::shared_ptr<Type> res_t;

    void accept(StmtVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override {}
  };

  inline std::shared_ptr<Type> get_res_t(const std::variant<TypeDef, TypeRef>& var)
  {
    return std::visit(
      Utils::overloaded
      {
        [&](const Node::TypeDef& def) -> std::shared_ptr<Type> { return def.res_t; },
        [&](const Node::TypeRef& ref) -> std::shared_ptr<Type> { return ref.res_t; }
      }, var
    );
  }

  struct Func;
  struct Struct;
  struct Union;
  struct Enum;
  struct Namespace;
  struct DeclVisitor
  {
    virtual ~DeclVisitor() = default;
    virtual void visit(const Func& func) = 0;
    virtual void visit(const Struct& strct) = 0;
    virtual void visit(const Union& un) = 0;
    virtual void visit(const Enum& en) = 0;
    virtual void visit(const Namespace& ns) = 0;
  };

  struct Param : Ident
  {
    explicit Param(TypeRef type, std::string id) : Ident(std::move(id)), type(std::move(type)) {}
    TypeRef type;
  };

  struct Func : Decl, Callable
  {
    explicit Func(Ident&& id, TypeRef rtype) : id(std::move(id)), ret_type(std::move(rtype)) {}
    void accept(DeclVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::Func: " << id.name << "(" << id.id << ")" << " {\n";
      
      if (!params.empty()) {
        std::cout << std::string(ident + 1, '\t') << "params: [\n";
        for (const auto& param : params) {
          param.dump(ident + 2);
        }
        std::cout << std::string(ident + 1, '\t') << "]\n";
      }

      std::cout << std::string(ident + 1, '\t') << "return: " << *ret_type.res_t << "\n";

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
    TypeRef ret_type;
  };

  struct Struct : Decl
  {
    explicit Struct(Ident id) : id(std::move(id)) {}
    void accept(DeclVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override {
      std::cout << std::string(ident, '\t') << "Node::Struct: " << id.name << " {\n";
      std::cout << std::string(ident + 1, '\t') << "SCOPE";
      /*
      for (const auto& [type, member_id] : members)
      {
        std::cout << std::string(ident + 1, '\t') << type << " " << member_id.name << "\n";
      }
      */
      std::cout << std::string(ident, '\t') << "}\n";
    }
    Ident id;
    // std::optional<Scope> scp;
    std::vector<std::pair<std::variant<TypeDef, TypeRef>, Ident>> members;
  };

  struct Union : Decl
  {
    explicit Union(Ident id) : id(std::move(id)) {}
    void accept(DeclVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override {
      std::cout << std::string(ident, '\t') << "Node::Union: " << id.name << " {\n";
      std::cout << std::string(ident + 1, '\t') << "SCOPE";
      /*
      for (const auto& [type, member_id] : members)
      {
        std::cout << std::string(ident + 1, '\t') << member_id.name << "\n";
      }
      */
      std::cout << std::string(ident, '\t') << "}\n";
    }
    Ident id;
    // std::optional<Scope> scp;
    std::vector<std::pair<std::variant<TypeDef, TypeRef>, Ident>> members;
  };

  struct Namespace : Decl
  {
    explicit Namespace(Ident id) : id(std::move(id)) {}
    void accept(DeclVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override
    {
      std::cout << std::string(ident, '\t') << "Node::Namespace: " << id.name << " {\n";
      std::cout << std::string(ident + 1, '\t') << "scp: \n";
      for (auto& term : scp.terms)
        std::visit(
          Utils::overloaded
          {
            [&](const std::shared_ptr<Expr>& expr) { expr->dump(ident + 2); },
            [&](const std::shared_ptr<Stmt>& stmt) { stmt->dump(ident + 2); },
            [&](const std::shared_ptr<Decl>& decl) { decl->dump(ident + 2); },
          }, term
        );
      /*
      for (const auto& [type, member_id] : members)
      {
        std::cout << std::string(ident + 1, '\t') << member_id.name << "\n";
      }
      */
      std::cout << std::string(ident, '\t') << "}\n";
    }
    Ident id;
    Scope scp;
  };

  struct Asgn : Stmt
  {
    Ident id;
    std::variant<TypeDef, TypeRef> type;
    std::optional<std::variant<std::shared_ptr<Expr>, std::shared_ptr<Decl>>> val;
    Asgn(std::variant<TypeDef, TypeRef> type, std::string id) : type(std::move(type)), id(std::move(id)), val() {}
    Asgn(std::variant<TypeDef, TypeRef> type, std::string id, std::shared_ptr<Expr> expr)
        : type(std::move(type)), id(std::move(id)), val(expr)
    {}
    void accept(StmtVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override;
  };

  struct Enum : Decl
  {
    explicit Enum(std::string id) : id(std::move(id)) {}
    void accept(DeclVisitor& v) override { v.visit(*this); }
    void dump(int ident) const override {
      std::cout << std::string(ident, '\t') << "Node::Enum: " << id.name << " {\n";
      for (const auto& en : enums)
      {
        std::cout << std::string(ident + 1, '\t') << en.id.name << "\n";
      }
      std::cout << std::string(ident, '\t') << "}\n";
    }
    Ident id;
    std::vector<Asgn> enums;
  };


  inline void Asgn::dump(int ident) const
  {
    std::cout << std::string(ident, '\t') << "Node::Asgn {\n";
    std::cout << std::string(ident + 1, '\t') << "type: " <<
    std::visit(
      Utils::overloaded
      {
        [](const TypeDef& td) -> std::string
        {
          return std::visit(
            Utils::overloaded
            {
              [](const std::shared_ptr<Node::Struct>& stc) -> std::string { return stc->id.name; },
              [](const std::shared_ptr<Node::Union>& uni) -> std::string { return uni->id.name; },
              [](const std::shared_ptr<Node::Enum>& enm) -> std::string {
  return enm->id.name; }
            }, td.type
          );
        },
        [](const TypeRef& tr) -> std::string { return Type::to_string(*tr.res_t); }
      }
    , type) << "\n";
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

  using Node = std::variant<std::shared_ptr<Stmt>, std::shared_ptr<Expr>, std::shared_ptr<Decl>>;
}

class Parser
{
private:
  class RollbackGuard
  {
  private:
    size_t& m_parser_idx;
    size_t m_saved_idx;
    bool m_committed;

  public:
    explicit RollbackGuard(size_t& parser_idx)
      : m_parser_idx(parser_idx), m_saved_idx(parser_idx), m_committed(false) {}

    void commit() noexcept { m_committed = true; }

    ~RollbackGuard() { if (!m_committed) m_parser_idx = m_saved_idx; }
    
    RollbackGuard(const RollbackGuard&) = delete;
    RollbackGuard& operator=(const RollbackGuard&) = delete;
  };

public:
  explicit Parser(std::vector<Token>& tokens);
  std::vector<Node::Node> parse_prog();
  std::optional<UnOp> parse_un_op();
  std::optional<UnOp> parse_pref_un_op();
  std::optional<UnOp> parse_suff_un_op();
  std::optional<BinOp> parse_bin_op();

  bool is_right_assoc(const BinOp& binop);
  bool is_right_assoc(const UnOp& unop);
  std::optional<std::shared_ptr<Node::Expr>> parse_paren_expr();
  std::optional<std::shared_ptr<Node::Expr>> parse_bin_expr(std::optional<std::shared_ptr<Node::Expr>> lhs, std::optional<BinOp> prev_op);
  // std::optional<std::shared_ptr<Node::Expr>> parse_un_expr();
  std::optional<std::shared_ptr<Node::Expr>>
    parse_expr(std::optional<std::shared_ptr<Node::Expr>> lhs = std::nullopt, std::optional<BinOp> prev_op = std::nullopt);

  std::optional<Node::ExprStmt> parse_expr_stmt();
  std::optional<std::shared_ptr<Node::Stmt>> parse_stmt();
  std::optional<Node::Scope> parse_scope();
  std::optional<Node::Struct> parse_struct();
  std::optional<Node::Union> parse_union();
  std::optional<Node::Enum> parse_enum();
  std::optional<Node::Func> parse_func();
  std::optional<Node::Namespace> parse_namespace();

  std::optional<Node::Int> parse_lit_int();
  std::optional<Node::Double> parse_lit_double();
  std::optional<Node::String> parse_lit_string();
  std::optional<Node::Char> parse_lit_char();
  std::optional<Node::Path> parse_lit_path();
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
  // std::optional<Node::Call> parse_call_expr();
  std::optional<Node::Node> parse_node();
  std::optional<Node::TypeRef> parse_type_ref();
  std::optional<Node::TypeDef> parse_type_def();
  std::optional<std::variant<Node::TypeDef, Node::TypeRef>> parse_type();
  void parse_arr(std::variant<Node::TypeDef, Node::TypeRef>& type);
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
  ScopeTracker m_tracker;
  size_t m_index;
};

#endif // PARSER_H
