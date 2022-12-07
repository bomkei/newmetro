#include "types/Object.h"
#include "types/Node.h"
#include "types/Token.h"
#include "Error.h"
#include "Utils.h"
#include "Evaluator.h"

Evaluator::CallStack::CallStack(Node* func)
    : func(func),
      is_returned(false),
      result(nullptr)
{
}

Evaluator::LoopContext::LoopContext(Node* node, Scope& scope)
    : node(node),
      result(nullptr),
      is_breaked(false),
      scope(scope)
{
}

void Evaluator::adjust_object_type(Object*& lhs, Object*& rhs)
{
  // if (lhs->type.kind > rhs->type.kind) {
  //   std::swap(lhs, rhs);
  // }

  //
  // 左右の型が違う : lhs != rhs
  if (!lhs->type.equals(rhs->type)) {
    //
    // int, float
    //  --> float, float
    if (lhs->type.kind == TYPE_Int && rhs->type.kind == TYPE_Float) {
      lhs = new ObjFloat((float)((ObjLong*)lhs)->value);
    }

    //
    // float, int
    //  --> float, float
    else if (lhs->type.kind == TYPE_Float &&
             rhs->type.kind == TYPE_Int) {
      rhs = new ObjFloat((float)((ObjLong*)rhs)->value);
    }
  }
}

Object*& Evaluator::get_var(Token* name)
{
  for (auto&& scope : this->scope_stack) {
    for (auto&& var : scope.variables) {
      if (var.name == name->str) {
        return var.value;
      }
    }
  }

  // find func and create func obj
  for (auto&& scope : this->scope_stack) {
    for (auto&& x : scope.node->list) {
      if (x->kind == ND_Function &&
          x->nd_func_name->str == name->str) {
        if (this->func_obj_map.contains(x)) {
          return (Object*&)this->func_obj_map[x];
        }

        return (Object*&)this->func_obj_map[x] = new ObjFunction(x);
      }
    }
  }

  Error(ERR_UndefinedVariable, name).emit().exit();
}

Evaluator::Scope& Evaluator::enter_scope(Node* node)
{
  return this->scope_stack.emplace_front(node);
}

void Evaluator::leave_scope()
{
  auto& scope = this->get_cur_scope();

  for (auto&& v : scope.variables) {
    v.value->ref_count--;
  }

  this->scope_stack.pop_front();
}

Evaluator::LoopContext& Evaluator::get_cur_loop_context()
{
  return *this->loop_stack.begin();
}

void Evaluator::loop_continue()
{
  auto& scope = this->get_cur_scope();

  scope.is_skipped = true;
}

void Evaluator::loop_break()
{
  auto& scope = this->get_cur_scope();

  scope.is_skipped = true;
}

Evaluator::Scope& Evaluator::get_cur_scope()
{
  return *this->scope_stack.begin();
}

Evaluator::CallStack& Evaluator::get_cur_call_stack()
{
  return *this->call_stack.begin();
}

void Evaluator::check_user_func_args(Node* node, Node* nd_func,
                                     Scope& scope)
{
  auto iter_formal = nd_func->list.begin();

  auto iter_actual = scope.variables.begin();
  auto iter_actual_nd = node->list.begin();

  for (; iter_actual != scope.variables.end();
       iter_actual++, iter_formal++, iter_actual_nd++) {
    if ((*iter_formal)->kind == ND_VariableArguments) {
      return;
    }

    if (iter_formal == nd_func->list.end()) {
      Error(ERR_TooManyArguments, *iter_actual_nd).emit().exit();
    }
  }

  if (iter_formal != nd_func->list.end() &&
      (*iter_formal)->kind != ND_VariableArguments) {
    Error(ERR_TooFewArguments, node).emit().exit();
  }
}
