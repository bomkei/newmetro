#pragma once

#include <vector>
#include <functional>

struct Object;
struct Node;

struct BuiltinFunc {
  using FuncType =
      std::function<Object*(Node*, std::vector<Object*> const&)>;

  char const* name;
  FuncType func;

  BuiltinFunc(char const* name, FuncType func);

  static std::vector<BuiltinFunc> const builtin_functions;
};
