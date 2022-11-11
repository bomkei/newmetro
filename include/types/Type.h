#pragma once

#include <string>
#include <vector>

enum TypeKind {
  TYPE_None,
  TYPE_Int,
  TYPE_Float,
  TYPE_Bool,
  TYPE_Char,
  TYPE_String,
  TYPE_Tuple,
  TYPE_Vector,
  TYPE_Range,
  TYPE_Args,
  TYPE_Function
};

struct Type {
  TypeKind kind;
  bool is_mutable;
  bool is_reference;

  std::vector<Type> elements;

  Type(TypeKind kind = TYPE_None)
      : kind(kind),
        is_mutable(false),
        is_reference(false)
  {
  }

  Type(TypeKind kind, bool is_mutable, bool is_reference)
      : kind(kind),
        is_mutable(is_mutable),
        is_reference(is_reference)
  {
  }

  std::string to_string() const;

  bool equals(Type const& type) const;
};
