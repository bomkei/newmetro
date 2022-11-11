#include <cassert>
#include "types/Type.h"

static char const* typename_list[]{
    "none",  "int", "float", "bool", "char", "string",
    "tuple", "vec", "range", "args", "func",
};

std::string Type::to_string() const
{
  assert(static_cast<int>(this->kind) < std::size(typename_list));

  return typename_list[static_cast<int>(this->kind)];
}

bool Type::equals(Type const& type) const
{
  if (this->kind == type.kind) {
    return true;
  }

  return false;
}