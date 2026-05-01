#ifndef SEMA_H
#define SEMA_H

#include "parser.hpp"
#include "tokenizer.hpp"
#include "type.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

class Sema
{
  /*
  struct Visitor : Node::DeclVisitor, Node::ExprVisitor, Node::StmtVisitor
  {
    Sema& m_sema;
    explicit Visitor(Sema& sema) : m_sema(sema) {}

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
    void visit(const Node::Ret&) override;
    void visit(const Node::ExprStmt&) override;
  };
  */

public:
  struct Symbol;
  struct VarExt;

  struct Symbol
  {
    struct FnExt
    {
      std::string mang_name;
      std::vector<Symbol> params;
      FnExt(const std::string& mname, const std::vector<Node::Param>& prms);
    };

    struct StcExt
    {
      uint32_t tot_sz;
      explicit StcExt(uint32_t sz) : tot_sz(sz) {};
    };

    struct VarExt
    {
      bool is_const = false;
      bool is_global = false;
      explicit VarExt() = default;
      explicit VarExt(bool is_g) : is_global(is_g) {};
    };

    static uint32_t nid;
    using Ext = std::variant<FnExt, StcExt, VarExt>;
    uint32_t id;
    Type type;
    Ext ext;
    explicit Symbol(Type type, Ext ext) : id(nid++), type(std::move(type)), ext(std::move(ext)) {}
    [[nodiscard]] bool is_fn() const { return std::holds_alternative<FnExt>(ext); }
    [[nodiscard]] bool is_st() const { return std::holds_alternative<StcExt>(ext); }
    [[nodiscard]] bool is_vr() const { return std::holds_alternative<VarExt>(ext); }
  };

  enum class ValCat
  {
    LVALUE,
    RVALUE,
  };

  struct ExprInfo
  {
    explicit ExprInfo(Type type, ValCat val_cat) : type(std::move(type)), val_cat(val_cat) {}
    Type type;
    ValCat val_cat;
  };

  struct Scope
  {
    explicit Scope() : prev(nullptr), next(nullptr) {};
    Scope* prev;
    Scope* next;
    std::unordered_map<std::string, Sema::Symbol> m_sym_table;
    std::unordered_set<Type::Custom> m_types_table;
  };

  struct Module
  {
    Module* parent;
    std::vector<Module> children;
    std::vector<std::string> symbols;
  };

  struct Analysis
  {
    // maps parser ident id to specific symbol
    std::unordered_map<uint32_t, Sema::Symbol> sym_table;
  };

public:
  explicit Sema(const std::vector<Node::Node>&);
  ~Sema();
  Sema::Analysis analyze();

  void analyze_type_usage(const std::shared_ptr<Type>&);
  ExprInfo analyze_expr(const std::shared_ptr<Node::Expr>&);
  ExprInfo analyze_lit(const std::shared_ptr<Node::Lit>&);
  void analyze_decl(const std::shared_ptr<Node::Decl>&);
  void analyze_stmt(const std::shared_ptr<Node::Stmt>&);

  void analyze_scope(const Node::Scope&);
  void analyze_func(const Node::Func&);
  void analyze_struct(const Node::Struct&);

  ExprInfo analyze_bin_expr(const Node::BinExpr&);
  ExprInfo analyze_un_expr(const Node::UnExpr&);
  ExprInfo analyze_ident(const Node::Ident&);
  ExprInfo analyze_int(const Node::Int&);
  ExprInfo analyze_call(const Node::Call&);

  void analyze_asgn(const Node::Asgn&);
  void analyze_if(const Node::If&);
  void analyze_for(const Node::For&);
  void analyze_ret(const Node::Ret&);
  ExprInfo analyze_expr_stmt(const Node::ExprStmt&);

private:
  bool is_valid_type(const Type&);
  Scope* get_curr_scope();
  Module* load_module();
  void pop_scope();

private:
  Sema::Analysis g_anl;
  // Visitor m_visitor;
  std::vector<std::unique_ptr<Scope>> m_scope_stack;
  const std::vector<Node::Node>& m_nodes;
};

inline Sema::Symbol::FnExt::FnExt(const std::string& mname, const std::vector<Node::Param>& prms) : mang_name(mname)
{
  for (auto& prm : prms)
    params.emplace_back(prm.type, VarExt());
};

#endif // SEMA_H
