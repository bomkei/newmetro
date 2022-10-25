#include "metro.h"

#define blambda(e) \
  [](Node * node, std::vector<Object*> const& args) -> Object* e

BuiltinFunc::BuiltinFunc(char const* name, FuncPointer func)
    : name(name),
      func(func)
{
}

static auto const bfun_print = blambda({
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
    {"println", blambda({
       auto ret = bfun_print(node, args);

       std::cout << std::endl;

       ((ObjLong*)ret)->value++;
       return ret;
     })},

    // append
    {"append", blambda({
       if (args.size() < 2 || !args[0]->type.equals(TYPE_Vector)) {
         Error(ERR_IllegalFunctionCall, node).emit().exit();
       }

       for (auto it = args.begin() + 1; it != args.end(); it++)
         ((ObjVector*)args[0])->list.emplace_back(*it);

       return args[0];
     })},
};
