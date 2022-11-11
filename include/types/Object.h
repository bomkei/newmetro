#pragma once

#include <string>
#include <vector>

struct Type;

struct Object {
  Type type;
  size_t ref_count;

  virtual std::string to_string() const = 0;
  virtual Object* clone() const = 0;

 protected:
  Object(Type const& type);
};

struct ObjNone : Object {
  ObjNone();

  std::string to_string() const override;
  ObjNone* clone() const override;
};

template <class T, TypeKind kind>
struct ObjImmediate : Object {
  T value{};

  ObjImmediate()
      : Object(kind)
  {
  }

  ObjImmediate(T val)
      : Object(kind),
        value(val)
  {
  }

  std::string to_string() const override
  {
    return std::to_string(this->value);
  }

  ObjImmediate* clone() const override
  {
    return new ObjImmediate<T, kind>(this->value);
  }
};

using ObjLong = ObjImmediate<int64_t, TYPE_Int>;
using ObjChar = ObjImmediate<wchar_t, TYPE_Char>;
using ObjBool = ObjImmediate<bool, TYPE_Bool>;

template <>
std::string ObjImmediate<bool, TYPE_Bool>::to_string() const;

struct ObjString : Object {
  std::wstring value;

  ObjString(std::wstring&& val = L"")
      : Object(TYPE_String),
        value(std::move(val))
  {
  }

  ObjString(std::wstring const& val)
      : Object(TYPE_String),
        value(val)
  {
  }

  void append(wchar_t ch)
  {
    this->value.push_back(ch);
  }

  void append(std::wstring const& s)
  {
    this->value.append(s);
  }

  std::string to_string() const override;
  ObjString* clone() const override;
};

struct ObjFloat : Object {
  float value;

  ObjFloat(float val = 0)
      : Object(TYPE_Float),
        value(val)
  {
  }

  std::string to_string() const override;
  ObjFloat* clone() const override;
};

template <TypeKind k, char begin, char end>
struct ObjList : Object {
  std::vector<Object*> elements;

  ObjList()
      : Object(k)
  {
  }

  std::string to_string() const override;
  ObjList* clone() const override;
};

using ObjTuple = ObjList<TYPE_Tuple, '(', ')'>;
using ObjVector = ObjList<TYPE_Vector, '[', ']'>;

struct ObjRange : Object {
  using ValueType = int64_t;

  ValueType begin;
  ValueType end;

  ObjRange(ValueType begin, ValueType end)
      : Object(TYPE_Range),
        begin(begin),
        end(end)
  {
  }

  std::string to_string() const
  {
    return Utils::format("range(%lu, %lu)", this->begin, this->end);
  }

  ObjRange* clone() const
  {
    return new ObjRange(begin, end);
  }
};

struct Node;
struct BuiltinFunc;
struct ObjFunction : Object {
  bool is_builtin;
  Node* func;
  BuiltinFunc const* builtin;

  ObjFunction(Node* func)
      : Object(TYPE_Function),
        is_builtin(false),
        func(func),
        builtin(nullptr)
  {
  }

  std::string to_string() const override
  {
    return Utils::format(
        "<%sfunc 0x%X>", this->builtin ? "builtin-" : "",
        std::hash<void*>()(this->is_builtin ? (void*)this->builtin
                                            : this->func));
  }

  ObjFunction* clone() const override
  {
    return new ObjFunction(*this);
  }

  static ObjFunction* from_builtin(BuiltinFunc const& b)
  {
    auto x = new ObjFunction(nullptr);

    x->is_builtin = true;
    x->builtin = &b;

    return x;
  }
};
