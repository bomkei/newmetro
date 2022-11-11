#pragma once

#include <string>
#include <vector>

#include "Type.h"

struct Node;
struct BuiltinFunc;

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

template <TypeKind k, char begin, char end>
struct ObjList : Object {
  std::vector<Object*> elements;

  ObjList();

  std::string to_string() const override;
  ObjList* clone() const override;
};

struct ObjString : Object {
  std::wstring value;

  ObjString(std::wstring&& val = L"");
  ObjString(std::wstring const& val);

  void append(wchar_t ch);
  void append(std::wstring const& s);

  std::string to_string() const override;
  ObjString* clone() const override;
};

struct ObjFloat : Object {
  float value;

  ObjFloat(float val = 0);

  std::string to_string() const override;
  ObjFloat* clone() const override;
};

struct ObjRange : Object {
  using ValueType = int64_t;

  ValueType begin;
  ValueType end;

  ObjRange(ValueType begin, ValueType end);

  std::string to_string() const override;
  ObjRange* clone() const override;
};

struct ObjFunction : Object {
  bool is_builtin;
  Node* func;
  BuiltinFunc const* builtin;

  ObjFunction(Node* func);

  std::string to_string() const override;
  ObjFunction* clone() const override;

  static ObjFunction* from_builtin(BuiltinFunc const& b);
};

using ObjLong = ObjImmediate<int64_t, TYPE_Int>;
using ObjChar = ObjImmediate<wchar_t, TYPE_Char>;
using ObjBool = ObjImmediate<bool, TYPE_Bool>;

using ObjTuple = ObjList<TYPE_Tuple, '(', ')'>;
using ObjVector = ObjList<TYPE_Vector, '[', ']'>;

template <>
std::string ObjImmediate<bool, TYPE_Bool>::to_string() const;
