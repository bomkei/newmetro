#include "metro.h"

#define blambda(e) [](Node * node, BF_Args const& args) -> Object* e

using BF_Args = std::vector<Object*>;

class BuiltinBuilder {
 public:
  static BuiltinFunc create(char const* name,
                            std::vector<Type> const& arg_types,
                            BuiltinFunc::FuncType fp)
  {
    // dont free instance !
    auto builder = new BuiltinBuilder(arg_types, fp);

    return BuiltinFunc{name, builder->wrapper};
  }

 private:
  BuiltinBuilder(std::vector<Type> const& args,
                 BuiltinFunc::FuncType fp)
      : arg_types(args),
        _actual_func(fp)
  {
    this->wrapper =
        std::bind(&BuiltinBuilder::call_wrap, this,
                  std::placeholders::_1, std::placeholders::_2);
  }

  Object* call_wrap(Node* node,
                    std::vector<Object*> const& actual_args)
  {
    if (this->arg_types.empty() && !actual_args.empty()) {
      Error(ERR_TooManyArguments, node).emit().exit();
    }

    auto nd_arg = node->list.begin();
    auto formal = this->arg_types.begin();

    for (auto&& act_obj : actual_args) {
      // if end
      if (formal == this->arg_types.end()) {
        Error(ERR_TooManyArguments, node)
            .suggest(*nd_arg, "don't need this argument")
            .emit()
            .exit();
      }

      if (formal->equals(TYPE_Args)) {
        break;
      }

      // no matching type
      if (!formal->equals(act_obj->type)) {
        Error(ERR_IllegalFunctionCall, *nd_arg)
            .suggest(*nd_arg, "expected `" + formal->to_string() +
                                  "`, but found `" +
                                  act_obj->type.to_string() + "`")
            .emit()
            .exit();
      }

      formal++;
      nd_arg++;
    }

    if (formal != this->arg_types.end() && !formal->equals(TYPE_Args))
      Error(ERR_TooFewArguments, node).emit().exit();

    return this->_actual_func(node, actual_args);
  }

  BuiltinFunc::FuncType wrapper;
  std::vector<Type> arg_types;

  BuiltinFunc::FuncType _actual_func;
};

BuiltinFunc::BuiltinFunc(char const* name, BuiltinFunc::FuncType func)
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
  // if (args.empty() || !args[0]->type.equals(TYPE_String)) {
  //   Error(ERR_IllegalFunctionCall, node).emit().exit();
  // }

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
            // todo: impl other format specifiers

          case '}':
            ret->append(
                Utils::Converter::to_wide((*it++)->to_string()));

            c += 2;
            continue;
        }

        break;
      }
    }

    ret->append(*c++);
  }

  return ret;
}

}  // namespace

std::vector<BuiltinFunc> const BuiltinFunc::builtin_functions = {
    // abs
    BuiltinBuilder::create(
        "abs", {TYPE_Int}, blambda({
          return new ObjLong(std::abs(((ObjLong*)args[0])->value));
        })),

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
    BuiltinBuilder::create("format", {TYPE_String, TYPE_Args},
                           bf_format),

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
