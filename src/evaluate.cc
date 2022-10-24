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
    __none:
      return gcnew<ObjNone>();

    case ND_Function: {
      return gcnew<ObjFunction>(node);
    }

    case ND_True:
      return gcnew<ObjBool>(true);

    case ND_False:
      return gcnew<ObjBool>(false);

    case ND_Value:
      return node->nd_value->clone();

    case ND_Variable: {
      return this->eval_lvalue(node)->clone();
    }

    // call function
    case ND_Callfunc: {
      auto functor =
          (ObjFunction*)this->eval(node->nd_callfunc_functor);

      // not a function
      if (!functor->type.equals(TYPE_Function)) {
        Error(ERR_TypeMismatch, node->token).emit().exit();
      }

      auto& scope = this->enter_scope(functor->func);

      for (auto formal = functor->func->list.begin();
           auto&& arg : node->list) {
        auto& var = scope.variables.emplace_back(
            this->eval(arg), (*formal++)->nd_arg_name->str);
      }

      auto result = this->eval(functor->func->nd_func_code);

      this->leave_scope();

      return result;
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

    case ND_Bigger:
    case ND_BiggerOrEqual:
    case ND_Equal:
    case ND_NotEqual: {
      std::list<Node*> items{node};
      Node* x = node->nd_lhs;

      for (; x->kind >= ND_Bigger && x->kind <= ND_NotEqual;
           x = x->nd_lhs) {
        items.emplace_front(x);
      }

      Object* lhs = this->eval(x);

      for (auto&& item : items) {
        auto rhs = this->eval(item->nd_rhs);

        switch (item->kind) {
          case ND_Bigger:
            switch (lhs->type.kind) {
              case TYPE_Int:
                if (((ObjLong*)lhs)->value <= ((ObjLong*)rhs)->value)
                  goto _fail;

                break;
            }
            break;
        }

        lhs = rhs;
      }

      return gcnew<ObjBool>(true);

    _fail:
      return gcnew<ObjBool>(false);
    }

    case ND_Assign: {
      auto& dest = this->eval_lvalue(node->nd_lhs);

      dest = this->eval(node->nd_rhs);

      return dest;
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

  Error(ERR_ExpectedLeftValue, node).emit().exit();
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
