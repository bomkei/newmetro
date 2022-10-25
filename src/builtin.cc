#include "metro.h"

#define definition(e) \
  [](std::vector<Object*> const& args) -> Object* { e }

BuiltinFunc::BuiltinFunc(char const* name, FuncPointer func)
    : name(name),
      func(func)
{
}

static auto const bfun_print = definition({
  auto ret = new ObjLong;

  for (auto&& arg : args) {
    auto&& s = arg->to_string();

    ret->value += s.length();

    std::cout << s;
  }

  return ret;
});

std::vector<BuiltinFunc> const BuiltinFunc::builtin_functions = {
    // print
    {"print", bfun_print},

    // println
    {"println", definition({
       auto ret = bfun_print(args);

       std::cout << std::endl;

       ((ObjLong*)ret)->value++;
       return ret;
     })},

    // append
};
