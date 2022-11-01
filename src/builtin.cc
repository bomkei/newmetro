#include "metro.h"

#define blambda(e) \
  [](Node * node, std::vector<Object*> const& args) -> Object* e

BuiltinFunc::BuiltinFunc(char const* name, FuncPointer func)
    : name(name),
      func(func)
{
}

namespace {

// print
auto const bfun_print = blambda({
  auto ret = new ObjLong;

  for (auto&& arg : args) {
    auto&& s = arg->to_string();

    ret->value += s.length();

    std::cout << s;
  }

  return ret;
});

// format
auto const bfun_format = blambda({
  if (args.empty() || !args[0]->type.equals(TYPE_String)) {
    Error(ERR_IllegalFunctionCall, node).emit().exit();
  }

  auto it = args.begin() + 1;
  auto fmt = (ObjString*)args[0];

  auto ret = new ObjString;

  for (auto c = fmt->value.cbegin(), e = fmt->value.cend(); c != e;) {
    switch (*c) {
      case '{': {
        if (it == args.end()) {
          Error(ERR_TooFewArguments, node).emit().exit();
        }

        switch (c[1]) {
            // todo: format specifier

          case '}':
            ret->append(
                Utils::Converter::to_wide((*it++)->to_string()));

            c += 2;
            continue;
        }
      }
    }

    ret->append(*c++);
  }

  return ret;
});

}  // namespace

std::vector<BuiltinFunc> const BuiltinFunc::builtin_functions = {
    // abs
    {"abs", blambda({
       if (args.size() != 1 || !args[0]->type.equals(TYPE_Int))
         Error(ERR_IllegalFunctionCall, node).emit().exit();

       return new ObjLong(std::abs(((ObjLong*)args[0])->value));
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

    // format
    {"format", bfun_format},

    // print
    {"print", bfun_print},

    // println
    {"println", blambda({
       auto ret = bfun_print(node, args);

       std::cout << std::endl;

       ((ObjLong*)ret)->value++;
       return ret;
     })},
};
