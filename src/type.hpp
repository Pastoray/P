#ifndef TYPE_H
#define TYPE_H

#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <string>
#include <memory>
#include <variant>
#include <optional>
#include <unordered_map>
#include <vector>

struct Type
{
  enum class Base
  {
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    STR,
    F32,
    F64,
    BYTE,
    VOID,
  };
  struct Struct
  {
    explicit Struct(std::string name) : name(std::move(name)), body(std::make_shared<Body>()) {}
    std::string name;
    struct Body
    {
      std::unordered_map<std::string, size_t> offsets;
      std::unordered_map<std::string, std::shared_ptr<Type>> types;
    };

    std::shared_ptr<Body> body;
    bool operator==(const Struct& other) const { return name == other.name; };

    void insert_mem(std::shared_ptr<Type> tp, const std::string& mem)
    {
      assert(!body->offsets.count(mem) && "Duplicated member");
      size_t cur_off = std::accumulate(
        body->types.begin(), body->types.end(), 0LL,
        [&](long long off, const std::pair<std::string, std::shared_ptr<Type>>& p)
        { return off + p.second->size(); }
      );
      body->offsets[mem] = cur_off;
      body->types.try_emplace(mem, tp);
    }
  };

  struct Union
  {
    explicit Union(std::string name) : name(std::move(name)), body(std::make_shared<Body>()) {}
    std::string name;
    struct Body
    {
      std::unordered_map<std::string, size_t> offsets;
      std::unordered_map<std::string, std::shared_ptr<Type>> types;
    };

    std::shared_ptr<Body> body;
    bool operator==(const Union& other) const { return name == other.name; };

    void insert_mem(std::shared_ptr<Type> tp, const std::string& mem)
    {
      assert(!body->offsets.count(mem) && "Duplicated member");
      body->offsets[mem] = 0;
      body->types.try_emplace(mem, tp);
    }
  };

  struct Enum
  {
    explicit Enum(std::string name) : name(std::move(name)) {}
    std::string name;
    // std::shared_ptr<Type> tag;
    std::unordered_map<std::string, std::shared_ptr<void>> enums;
    bool operator==(const Enum& other) const { return name == other.name; };
  };

  struct Ptr
  {
    explicit Ptr(std::shared_ptr<Type> to) : to(std::move(to)) {}
    std::shared_ptr<Type> to;
    bool operator==(const Ptr& other) const { return *to == *other.to; }
  };
  
  struct Arr
  {
    explicit Arr(std::shared_ptr<Type> of, std::shared_ptr<void> sz) : of(std::move(of)), size(std::move(sz)) {}
    std::shared_ptr<Type> of;
    std::shared_ptr<void> size;
    bool operator==(const Arr& other) const { return *of == *other.of && size == other.size; }
  };

  using TypeVar = std::variant<Base, Ptr, Arr, Struct, Union, Enum>;
  TypeVar type;
  explicit Type(std::string id)
  {
    if (auto base_t = string_to_base_t(id))
    {
      type = *base_t;
      return;
    }
    type = Struct{ std::move(id) };
  }

  explicit Type (TypeVar tp)
  { type = std::move(tp); }

  [[nodiscard]] bool is_base_t() const
  {
    return std::holds_alternative<Base>(type);
  }

  [[nodiscard]] bool is_arr_t() const
  {
    return std::holds_alternative<Arr>(type);
  }

  [[nodiscard]] bool is_ptr_t() const
  {
    return std::holds_alternative<Ptr>(type);
  }

  [[nodiscard]] bool is_struct_t() const
  {
    return std::holds_alternative<Struct>(type);
  }

  [[nodiscard]] bool is_union_t() const
  {
    return std::holds_alternative<Union>(type);
  }

  [[nodiscard]] bool is_enum_t() const
  {
    return std::holds_alternative<Enum>(type);
  }

  [[nodiscard]] bool is_int() const
  {
    if (auto* bt = std::get_if<Base>(&type))
      return *bt >= Base::I8 && *bt <= Base::U64;
    return false;
  }
  [[nodiscard]] bool is_signed() const
  {
    if (auto* bt = std::get_if<Base>(&type))
      return *bt >= Base::I8 && *bt <= Base::I64;
    return false;
  }
  [[nodiscard]] bool is_float() const
  {
    if (auto* bt = std::get_if<Base>(&type))
      return *bt == Base::F32 || *bt == Base::F64;
    return false;
  }
   bool operator==(const Type& other) const { return type == other.type; }
  [[nodiscard]] size_t size() const
  {
    return std::visit(
      Utils::overloaded
      {
        [](const Base& bt) -> size_t
        {
          switch (bt)
          {
            case Base::VOID:
              return 0;

            case Base::I8:
            case Base::U8:
            case Base::BYTE:
              return 1;

            case Base::I16:
            case Base::U16:
              return 2;

            case Base::I32:
            case Base::U32:
            case Base::F32:
              return 4;

            case Base::I64:
            case Base::U64:
            case Base::F64:
            case Base::STR:
              return 8;

            default:
              return 0;
          }
        },
        [](const Ptr& t) -> size_t
        {
          return 8;
        },
        [](const Arr& t) -> size_t
        {
          return 8;
        },
        [](const Struct& st) -> size_t
        {
          size_t tot = std::accumulate(
            st.body->types.begin(), st.body->types.end(), 0LL,
            [&](long long off, const std::pair<std::string, std::shared_ptr<Type>>& p) { return off + p.second->size(); }
          );
          return tot;
        },
        [](const Union& un) -> size_t
        {
          auto it = std::max_element(
            un.body->types.begin(), un.body->types.end(),
            [&](const auto& a, const auto& b) { return a.second->size() < b.second->size(); }
          );
          return it->second->size();
        },
        [](const Enum& en) -> size_t
        {
          return 4;
        },
      }, type
    );
  }

  [[nodiscard]] std::shared_ptr<Type> inner() const
  {
    return std::visit(
      Utils::overloaded
      {
        [](const Base& bt) -> std::shared_ptr<Type> { assert(false && "BaseType does not have inner type"); return nullptr; },
        [](const Ptr& t) -> std::shared_ptr<Type>
        {
          return t.to;
        },
        [](const Arr& t) -> std::shared_ptr<Type>
        {
          return t.of;
        },
        [](const Struct& st) -> std::shared_ptr<Type>
        {
          assert(false && "type.inner() on struct is ambiguous");
          return nullptr;
        },
        [](const Union& un) -> std::shared_ptr<Type>
        {
          assert(false && "type.inner() on union is ambiguous");
          return nullptr;
        },
        [](const Enum& en) -> std::shared_ptr<Type>
        {
          assert(false && "type.inner() on enum is ambiguous");
          return nullptr;
        },
      }, type
    );
  }

  static std::optional<Base> string_to_base_t(const std::string& str)
  {
    if (str == "i8")
      return Base::I8;
    else if (str == "i16")
      return Base::I16;
    else if (str == "i32")
      return Base::I32;
    else if (str == "i64")
      return Base::I64;
    else if (str == "u8")
      return Base::U8;
    else if (str == "u16")
      return Base::U16;
    else if (str == "u32")
      return Base::U32;
    else if (str == "u64")
      return Base::U64;
    else if (str == "str")
      return Base::STR;
    else if (str == "f32")
      return Base::F32;
    else if (str == "f64")
      return Base::F64;
    else if (str == "byte")
      return Base::BYTE;
    else if (str == "void")
      return Base::VOID;
    return {};
  }

  static std::string to_string(const Base& type)
  {
    switch (type)
    {
      case Base::I8:   return "I8";
      case Base::I16:  return "I16";
      case Base::I32:  return "I32";
      case Base::I64:  return "I64";
      case Base::U8:   return "U8";
      case Base::U16:  return "U16";
      case Base::U32:  return "U32";
      case Base::U64:  return "U64";
      case Base::STR:  return "STR";
      case Base::F32:  return "F32";
      case Base::F64:  return "F64";
      case Base::BYTE: return "BYTE";
      case Base::VOID: return "VOID";
      default: return "UNKNOWN";
    }
  }

  static std::string to_string(const Type::Ptr& type)
  {
    return Type::to_string(*type.to) + "*";
  }

  static std::string to_string(const Type::Struct& type)
  {
    return type.name;
  }
  
  static std::string to_string(const Type::Union& type)
  {
    return type.name;
  }

  static std::string to_string(const Type::Enum& type)
  {
    return type.name;
  }

  static std::string to_string(const Type::Arr& type)
  {
    return Type::to_string(*type.of) + "[" /* + std::to_string(type.size) */ + "]";
  }

  static std::string to_string(const Type& type)
  {
    return std::visit([&](auto&& arg) { return Type::to_string(arg); }, type.type);
  }

  friend std::ostream& operator<<(std::ostream& os, const Type& t)
  {
    os << Type::to_string(t);
    return os;
  }

  [[nodiscard]] int rank() const
  {
    return std::visit(
      Utils::overloaded
      {
        [](const Base& bt) -> int
        {
          if (bt == Base::F64) return 310;
          if (bt == Base::F32) return 305;
          if (bt >= Base::I8 && bt <= Base::I64) return 100 + Type(bt).size() + 5;
          if (bt >= Base::U8 && bt <= Base::U64) return 100 + Type(bt).size();
          if (bt == Base::BYTE) return 50;
          return 1;
        },
        [](const Ptr& ptr) -> int
        { return 400; },
        [](const Arr& arr) -> int
        { return 500; },
        [](const Struct& ct) -> int
        { return 1000; },
        [](const Union& un) -> int
        { return 999; },
        [](const Enum& en) -> int
        { return 100 + 4; }
      }, type
    );
  }
};

template <>
struct std::hash<Type::Struct>
{
  size_t operator()(const Type::Struct& t) const noexcept
  {
    return std::hash<std::string>{}(t.name);
  }
};

template <>
struct std::hash<Type::Union>
{
  size_t operator()(const Type::Union& t) const noexcept
  {
    return std::hash<std::string>{}(t.name);
  }
};

template <>
struct std::hash<Type::Enum>
{
  size_t operator()(const Type::Enum& t) const noexcept
  {
    return std::hash<std::string>{}(t.name);
  }
};

template <>
struct std::hash<Type::Ptr>
{
  size_t operator()(const Type::Ptr& t) const noexcept
  {
    return std::hash<Type*>{}(t.to.get());
  }
};

template <>
struct std::hash<Type::Arr>
{
  size_t operator()(const Type::Arr& t) const noexcept
  {
    auto h1 = std::hash<Type*>{}(t.of.get());
    // auto h2 = std::hash<size_t>{}(t.size);
    return h1; // ^ (h2 << 1);
  }
};

template <>
struct std::hash<Type>
{
  size_t operator()(const Type& t) const noexcept
  {
    size_t h = std::visit([](auto&& arg) { return std::hash<std::decay_t<decltype(arg)>>{}(arg); }, t.type);
    return h ^ (std::hash<size_t>{}(t.type.index()) << 1);
  }
};

#endif // TYPE_H
