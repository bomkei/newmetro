#include "metro.h"

#define blambda(e) [](Node * node, BF_Args const& args) -> Object* e

using BF_Args = std::vector<Object*>;

BuiltinFunc::BuiltinFunc(char const* name, FuncPointer func)
    : name(name),
      func(func)
{
}

namespace {

// print
Object* bf_print(Node* node, BF_Args const& args)
{
  auto ret = new ObjLong;

  for (auto&& arg : args) {
    auto&& s = arg->to_string();

    ret->value += s.length();

    std::cout << s;
  }

  return ret;
};

// format
Object* bf_format(Node* node, BF_Args const& args)
{
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
}

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
         ((ObjVector*)args[0])->elements.emplace_back(*it);

       return args[0];
     })},

    // format
    {"format", bf_format},

    // print
    {"print", bf_print},

    // printf
    {"printf", blambda({
       auto s = (ObjString*)bf_format(node, args);

       std::cout << s->to_string();

       auto ret = new ObjLong(s->value.length());

       return ret;
     })},

    // println
    {"println", blambda({
       auto ret = bf_print(node, args);

       std::cout << std::endl;

       ((ObjLong*)ret)->value++;
       return ret;
     })},
};
