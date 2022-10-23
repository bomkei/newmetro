#include "metro.h"

template <std::derived_from<Object> T, class... Args>
T* gcnew(Args&&... args)
{
  T* obj;

  if constexpr (sizeof...(args)) {
    obj = new T(std::forward<Args...>(args...));
  }
  else {
    obj = new T;
  }

  // todo: append

  return obj;
}

Object* Evaluator::mt_add(Object* lhs, Object* rhs)
{
  // todo: str mul int

  // todo: adjust type

  if (lhs->type.equals(TYPE_Vector)) {
    ((ObjVector*)lhs)->list.emplace_back(rhs);
  }

  switch (lhs->type.kind) {
    case TYPE_Int:
      ((ObjLong*)lhs)->value += ((ObjLong*)rhs)->value;
      break;
  }

  return lhs;
}

Object* Evaluator::mt_sub(Object* lhs, Object* rhs)
{
  switch (lhs->type.kind) {
    case TYPE_Int:
      ((ObjLong*)lhs)->value -= ((ObjLong*)rhs)->value;
      break;
  }

  return lhs;
}

Object* Evaluator::eval(Node* node)
{
  if (!node) {
    goto __none;
  }

  switch (node->kind) {
    case ND_None:
    case ND_Struct:
    case ND_Function:
    __none:
      return gcnew<ObjNone>();

    case ND_True:
      return gcnew<ObjBool>(true);

    case ND_False:
      return gcnew<ObjBool>(false);

    case ND_Value:
      return node->nd_value->clone();

    case ND_Variable: {
      return this->eval_lvalue(node)->clone();
    }

    case ND_Callfunc: {
      auto functor = node->nd_callfunc_functor;

      if (functor->kind == ND_Variable) {
        auto func = this->find_func(functor->token);

        // todo: check arguments

        auto& func_scope = this->enter_scope(func);

        for (auto act_arg_itr = node->list.begin();
             auto&& arg : func->list) {
          auto& var = func_scope.variables.emplace_back(
              this->eval(*act_arg_itr++), arg->nd_arg_name->str);
        }

        auto result = this->eval(func->nd_func_code);

        this->leave_scope();

        return result;
      }

      TODO_IMPL

      goto __none;
    }

    case ND_If: {
      auto cond = this->eval(node->nd_if_cond);

      if (!cond->type.equals(TYPE_Bool)) {
        Error(ERR_TypeMismatch, node->nd_if_cond)
            .suggest(node->nd_if_cond, "condition must boolean")
            .emit()
            .exit();
      }

      return this->eval(((ObjBool*)cond)->value ? node->nd_if_true
                                                : node->nd_if_false);
    }

    case ND_Let: {
      auto& scope = this->get_cur_scope();

      auto& var = scope.variables.emplace_back(
          nullptr, node->nd_let_name->str);

      if (node->nd_let_init) {
        var.value = this->eval(node->nd_let_init);
      }

      goto __none;
    }

    case ND_Scope: {
      auto& scope = this->enter_scope(node);
      Object* ret{};

      for (auto&& x : node->list) {
        ret = this->eval(x);

        if (scope.is_skipped) {
          ret = scope.lastval;
          break;
        }
      }

      this->leave_scope();

      return ret;
    }
  }

  auto lhs = this->eval(node->nd_lhs);
  auto rhs = this->eval(node->nd_rhs);

  switch (node->kind) {
    case ND_Add:
      lhs = this->mt_add(lhs, rhs);
      break;

    case ND_Sub:
      lhs = this->mt_sub(lhs, rhs);
      break;
  }

  return lhs;
}

Object*& Evaluator::eval_lvalue(Node* node)
{
  switch (node->kind) {
    case ND_Variable: {
      auto& ret = this->get_var(node->token);

      if (!ret) {
        Error(ERR_UninitializedVariable, node).emit().exit();
      }

      return ret;
    }
  }

  crash;
}

Evaluator::Scope& Evaluator::enter_scope(Node* node)
{
  return this->scope_stack.emplace_front(node);
}

void Evaluator::leave_scope() { this->scope_stack.pop_front(); }

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

Node* Evaluator::find_func(Token* name)
{
  for (auto&& scope : this->scope_stack) {
    for (auto&& item : scope.node->list) {
      if (item->kind == ND_Function &&
          item->nd_func_name->str == name->str) {
        return item;
      }
    }
  }

  Error(ERR_UndefinedFunction, name).emit().exit();
}