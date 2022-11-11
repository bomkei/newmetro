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
  if (lhs->type.kind > rhs->type.kind) {
    std::swap(lhs, rhs);
  }

  if (!lhs->type.equals(rhs->type)) {
    if (lhs->type.kind == TYPE_Int && rhs->type.kind == TYPE_Float) {
      lhs = new ObjFloat((float)((ObjLong*)lhs)->value);
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

  Error(ERR_UndefinedVariable, name).emit().exit();
}

Evaluator::Scope& Evaluator::enter_scope(Node* node)
{
  return this->scope_stack.emplace_front(node);
}

void Evaluator::leave_scope()
{
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
