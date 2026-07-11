#ifndef SEMA_H
#define SEMA_H

#include "parser.hpp"
#include "type.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

class Sema
{
public:
  struct Symbol;
  struct VarExt;
  struct Scope;

  struct Symbol
  {
    struct FnExt
    {
      std::string mang_name;
      std::vector<Symbol> params;
      FnExt(std::string mname, const std::vector<Node::Param>& prms) : mang_name(std::move(mname))
      {
        for (auto& prm : prms)
          params.emplace_back(*prm.type.res_t, VarExt());
      }
    };

    struct NsExt
    {
      uint32_t scp_id;
      explicit NsExt(uint32_t id) : scp_id(id) {}
    };
    struct StcExt {};
    struct UniExt {};
    struct EnuExt {};

    struct VarExt
    {
      bool is_const = false;
      bool is_global = false;
      std::string mang_name;
      explicit VarExt() = default;
      explicit VarExt(std::string mname, bool is_g) : is_global(is_g), mang_name(std::move(mname)) {};
    };

    static uint32_t nid;
    using Ext = std::variant<FnExt, StcExt, UniExt, EnuExt, VarExt, NsExt>;
    uint32_t id;
    Type type;
    Ext ext;
    explicit Symbol(Type type, Ext ext) : id(nid++), type(std::move(type)), ext(std::move(ext)) {}
    [[nodiscard]] bool is_fn() const { return std::holds_alternative<FnExt>(ext); }
    [[nodiscard]] bool is_st() const { return std::holds_alternative<StcExt>(ext); }
    [[nodiscard]] bool is_un() const { return std::holds_alternative<UniExt>(ext); }
    [[nodiscard]] bool is_en() const { return std::holds_alternative<EnuExt>(ext); }
    [[nodiscard]] bool is_vr() const { return std::holds_alternative<VarExt>(ext); }
    [[nodiscard]] bool is_ns() const { return std::holds_alternative<NsExt>(ext); }
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
    std::unordered_set<std::string> m_types_table;
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
  void push_scope();
  void pop_scope();

  explicit Sema(const std::vector<Node::Node>&);
  ~Sema();
  Sema::Analysis analyze();

  void analyze_type_ref(const Node::TypeRef& ref);
  void analyze_type_def(const Node::TypeDef& def);
  ExprInfo analyze_expr(const std::shared_ptr<Node::Expr>&);
  ExprInfo analyze_lit(const std::shared_ptr<Node::Lit>&);
  void analyze_decl(const std::shared_ptr<Node::Decl>&);
  void analyze_stmt(const std::shared_ptr<Node::Stmt>&);

  void analyze_scope(const Node::Scope&);
  void analyze_func(const Node::Func&);
  void analyze_namespace(const Node::Namespace&);

  ExprInfo analyze_bin_expr(Node::BinExpr&);
  ExprInfo analyze_un_expr(const Node::UnExpr&);
  ExprInfo analyze_ident(const Node::Ident&);
  ExprInfo analyze_path(const Node::Path&);
  ExprInfo analyze_int(const Node::Int&);
  ExprInfo analyze_bool(const Node::Bool&);
  ExprInfo analyze_double(const Node::Double&);
  ExprInfo analyze_string(const Node::String&);
  ExprInfo analyze_char(const Node::Char&);
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

private:
  Sema::Analysis g_anl;
  std::vector<uint32_t> m_scope_stack;
  std::vector<std::unique_ptr<Scope>> m_scope_arena;
  std::vector<std::string> m_mang_pref;
  std::unordered_map<std::string, Type> g_pre_sema;
  const std::vector<Node::Node>& m_nodes;
};

#endif // SEMA_H
