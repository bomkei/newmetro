#include "metro.h"

Evaluator::Scope& Evaluator::enter_scope(Node* node)
{
  return this->scope_stack.emplace_front(node);
}

void Evaluator::leave_scope() { this->scope_stack.pop_front(); }

void Evaluator::loop_continue()
{
  auto& scope = this->get_cur_scope();

  scope.is_skipped = true;
  scope.is_loop_continued = true;
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

Object*& Evaluator::eval_subscript(Node* node, Object* lhs,
                                   Object* index)
{
  if (!lhs->type.equals(TYPE_Vector)) {
    Error(ERR_TypeMismatch, node->nd_lhs)
        .suggest(node->nd_lhs, "expected vector")
        .emit()
        .exit();
  }

  if (!index->type.equals(TYPE_Int)) {
    Error(ERR_TypeMismatch, node->nd_rhs)
        .suggest(node->nd_rhs, "expected integer")
        .emit()
        .exit();
  }

  auto ival = ((ObjLong*)index)->value;

  if (ival < 0 || ival >= ((ObjVector*)lhs)->elements.size()) {
    Error(ERR_SubscriptOutOfRange, node->nd_rhs).emit().exit();
  }

  return ((ObjVector*)lhs)->elements[(unsigned)ival];
}
