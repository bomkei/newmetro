#include "metro.h"

template <>
std::string ObjImmediate<bool, TYPE_Bool>::to_string() const
{
  return this->value ? "true" : "false";
}
