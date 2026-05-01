#ifndef TYPE_H
#define TYPE_H

#include "utils.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <memory>
#include <variant>
#include <optional>

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
    BOOL,
    VOID,
  };
  struct Custom
  {
    explicit Custom(std::string name) : name(std::move(name)) {}
    std::string name;
    bool operator==(const Custom& other) const { return name == other.name; };
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

  std::variant<Base, Ptr, Arr, Custom> type;
  explicit Type (std::string id)
  {
    if (auto base_t = string_to_base_t(id))
    {
      type = *base_t;
      return;
    }
    type = Custom{ std::move(id) };
  }

  explicit Type (std::variant<Base, Ptr, Arr, Custom> tp)
  { type = std::move(tp); }
  /*
  explicit Type(Ident id) : name(std::move(id)), ptr(nullptr) {}
  explicit Type(std::string id) : name(Ident(std::move(id))), ptr(nullptr) {}
  explicit Type(Ident id, std::shared_ptr<Type> ptr) : name(std::move(id)), ptr(std::move(ptr)) {}
  explicit Type(std::string id, std::shared_ptr<Type> ptr) : name(Ident(std::move(id))), ptr(std::move(ptr)) {}
  */
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
  [[nodiscard]] uint32_t size() const
  {
    return std::visit(
      Utils::overloaded
      {
        [](const Base& bt) -> uint32_t
        {
          switch (bt)
          {
            case Base::VOID:
              return 0;

            case Base::I8:
            case Base::U8:
            case Base::BOOL:
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
        [](const Ptr& t) -> uint32_t
        {
          return 8;
        },
        [](const Arr& t) -> uint32_t
        {
          return 8;
        },
        [](const Custom& ct) -> uint32_t
        {
          assert(false && "NYI");
          return 0;
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
        [](const Custom& ct) -> std::shared_ptr<Type>
        {
          assert(false && "NYI");
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
    else if (str == "bool")
      return Base::BOOL;
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
      case Base::BOOL: return "BOOL";
      case Base::VOID: return "VOID";
      default: return "UNKNOWN";
    }
  }

  static std::string to_string(const Type::Ptr& type)
  {
    return Type::to_string(*type.to) + "*";
  }

  static std::string to_string(const Type::Custom& type)
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
          return 1;
        },
        [](const Ptr& ptr) -> int
        { return 400; },
        [](const Arr& arr) -> int
        { return 500; },
        [](const Custom& ct) -> int
        { return 1000; }
      }, type
    );
  }
};

template <>
struct std::hash<Type::Custom>
{
  size_t operator()(const Type::Custom& t) const noexcept
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
    // auto h2 = std::hash<uint32_t>{}(t.size);
    return h1; // ^ (h2 << 1);
  }
};

template <>
struct std::hash<Type>
{
  size_t operator()(const Type& t) const noexcept
  {
    size_t h = std::visit([](auto&& arg) { return std::hash<std::decay_t<decltype(arg)>>{}(arg); }, t.type);
    return h ^ (std::hash<uint32_t>{}(t.type.index()) << 1);
  }
};

#endif // TYPE_H
