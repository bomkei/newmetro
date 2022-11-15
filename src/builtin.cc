#include <iostream>

#include "types/Object.h"
#include "types/Node.h"
#include "types/Token.h"
#include "types/BuiltinFunc.h"
#include "Error.h"
#include "Utils.h"

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
            .emit();
      }

      formal++;
      nd_arg++;
    }

    if (formal != this->arg_types.end() && !formal->equals(TYPE_Args))
      Error(ERR_TooFewArguments, node).emit();

    Error::check();

    return this->_actual_func(node, actual_args);
  }

  std::vector<Type> arg_types;

  BuiltinFunc::FuncType wrapper;

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

    //
    // ---- constructors with type -----
    BuiltinBuilder::create("vector", {TYPE_Range}, blambda({
                             auto ret = new ObjVector();

                             auto R = (ObjRange*)args[0];

                             for (int64_t i = R->begin; i < R->end;
                                  i++) {
                               ret->append(new ObjLong(i));
                             }

                             return ret;
                           })),

    // print
    BuiltinBuilder::create("print", {TYPE_Args}, bf_print),

    // println
    BuiltinBuilder::create("println", {TYPE_Args}, blambda({
                             auto ret = bf_print(node, args);

                             std::cout << std::endl;

                             ((ObjLong*)ret)->value++;
                             return ret;
                           })),

    // printf
    BuiltinBuilder::create(
        "printf", {TYPE_String, TYPE_Args}, blambda({
          auto s = (ObjString*)bf_format(node, args);

          std::cout << s->to_string();

          auto ret = new ObjLong(s->value.length());

          return ret;
        })),
};
