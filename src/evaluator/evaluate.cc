#include <cassert>

#include "types/Object.h"
#include "types/Node.h"
#include "types/Token.h"
#include "types/BuiltinFunc.h"
#include "Error.h"
#include "Utils.h"
#include "Evaluator.h"

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

    case ND_Subscript: {
      return this->compute_subscript(node,
                                     this->eval_lvalue(node->nd_lhs),
                                     this->eval(node->nd_rhs));
    }
  }

  Error(ERR_TypeMismatch, node).emit().exit();
}

Object* Evaluator::eval_scope(Scope& scope, Node* node)
{
  Object* ret{};

  scope.is_skipped = false;

  for (auto&& x : node->list) {
    alert;

    ret = this->eval(x);

    if (scope.is_skipped) {
      break;
    }
  }

  return ret ? ret : gcnew<ObjNone>();
}

Object* Evaluator::eval(Node* node)
{
  if (!node) {
    goto __none;
  }

  alertfmt("eval(): node = %p, kind=%d", node, node->kind);

  switch (node->kind) {
    case ND_None:
    case ND_Struct:
    __none:
      return gcnew<ObjNone>();

    case ND_Value:
      return node->nd_value;

    case ND_True:
      return gcnew<ObjBool>(true);

    case ND_False:
      return gcnew<ObjBool>(false);

    case ND_EmptyList:
      return gcnew<ObjVector>();

    case ND_List: {
      auto ret = gcnew<ObjVector>();

      for (auto&& x : node->list) {
        ret->elements.emplace_back(this->eval(x));
      }

      return ret;
    }

    case ND_Function: {
      return gcnew<ObjFunction>(node);
    }

    case ND_SelfFunc: {
      if (this->call_stack.empty()) {
        Error(ERR_HereIsNotInsideOfFunc, node).emit().exit();
      }

      return gcnew<ObjFunction>(*this->call_stack.begin());
    }

    case ND_Variable: {
      for (auto&& bfun : BuiltinFunc::builtin_functions) {
        if (bfun.name == node->token->str) {
          return gcvia<ObjFunction>(ObjFunction::from_builtin(bfun));
        }
      }

      return this->eval_lvalue(node);
    }

    case ND_Subscript: {
      return this->compute_subscript(node, this->eval(node->nd_lhs),
                                     this->eval(node->nd_rhs));
    }

    //
    // call function
    case ND_Callfunc: {
      auto functor =
          (ObjFunction*)this->eval(node->nd_callfunc_functor);

      // not a function
      if (!functor->type.equals(TYPE_Function)) {
        Error(ERR_TypeMismatch, node->token).emit().exit();
      }

      if (functor->is_builtin) {
        std::vector<Object*> args;

        for (auto&& x : node->list) {
          args.emplace_back(this->eval(x));
        }

        return functor->builtin->func(node, args);
      }

      //
      // create a new scope for arguments
      auto& scope = this->enter_scope(functor->func);

      // callee
      auto callee = functor->func;

      // append call stack
      this->call_stack.emplace_front(callee);

      // create arguments
      for (auto formal = functor->func->list.begin();
           auto&& arg : node->list) {
        auto& var = scope.variables.emplace_back(
            this->eval(arg), (*formal++)->nd_arg_name->str);
      }

      auto result = this->eval(functor->func->nd_func_code);

      this->leave_scope();

      // remove call stack
      this->call_stack.pop_front();

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

    case ND_For: {
      auto& scope = this->enter_scope(node);

      auto& loopContext = this->loop_stack.emplace_front(node, scope);

      auto objTarget = this->eval(node->nd_for_range);

      switch (objTarget->type.kind) {
        case TYPE_Range: {
          auto objRange = (ObjRange*)objTarget;

          ObjLong counter{objRange->begin};

          if (node->nd_for_iterator->kind == ND_Variable) {
            scope.variables.emplace_back(
                &counter,
                node->nd_for_iterator->nd_variable_name->str);
          }

          for (auto begin = objRange->begin;
               !loopContext.is_breaked && begin < objRange->end;
               begin++, counter.value++) {
            this->eval_scope(scope, node->nd_for_loop_code);
          }

          break;
        }

        default:
          TODO_IMPL
      }

      auto result = loopContext.result;

      this->loop_stack.pop_front();

      this->leave_scope();

      if (!result) {
        goto __none;
      }

      return result;
    }

    case ND_Return: {
      auto& scope = this->get_cur_scope();

      TODO_IMPL

      break;
    }

    case ND_Break:
    case ND_Continue: {
      auto& loopContext = this->get_cur_loop_context();

      loopContext.scope.is_skipped = true;

      if (node->kind == ND_Break) {
        loopContext.is_breaked = true;

        if (node->nd_break_expr) {
          loopContext.result = this->eval(node->nd_break_expr);
        }
      }

      goto __none;
    }

    case ND_Let: {
      auto& scope = this->get_cur_scope();

      Variable* pvar{};

      if (!(pvar = scope.find_var(node->nd_let_name))) {
        pvar = &scope.variables.emplace_back(nullptr,
                                             node->nd_let_name->str);
      }

      assert(pvar);

      if (node->nd_let_init) {
        pvar->value = this->eval(node->nd_let_init);
      }

      goto __none;
    }

    case ND_Scope: {
      if (node->list.empty()) {
        goto __none;
      }

      Object* ret = this->eval_scope(this->enter_scope(node), node);

      this->leave_scope();

      return ret;
    }

    case ND_Range: {
      auto begin = this->eval(node->nd_lhs);
      auto end = this->eval(node->nd_rhs);

      if (!begin->type.equals(TYPE_Int))
        Error(ERR_TypeMismatch, node->nd_lhs)
            .suggest(node->nd_lhs, "expected integer")
            .emit()
            .exit();

      if (!end->type.equals(TYPE_Int))
        Error(ERR_TypeMismatch, node->nd_rhs)
            .suggest(node->nd_rhs, "expected integer")
            .emit()
            .exit();

      return gcnew<ObjRange>(((ObjLong*)begin)->value,
                             ((ObjLong*)end)->value);
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

        this->adjust_object_type(lhs, rhs);

        if (!lhs->type.equals(rhs->type)) {
          Error(ERR_TypeMismatch, node).emit().exit();
        }

        auto result = false;

        switch (item->kind) {
          case ND_Bigger:
            switch (lhs->type.kind) {
              case TYPE_Int:
                result =
                    ((ObjLong*)lhs)->value > ((ObjLong*)rhs)->value;
                break;

              case TYPE_Float:
                result =
                    ((ObjFloat*)lhs)->value > ((ObjFloat*)rhs)->value;
                break;
            }
            break;

          case ND_BiggerOrEqual:
            switch (lhs->type.kind) {
              case TYPE_Int:
                result =
                    ((ObjLong*)lhs)->value >= ((ObjLong*)rhs)->value;
                break;

              case TYPE_Float:
                result = ((ObjFloat*)lhs)->value >=
                         ((ObjFloat*)rhs)->value;
                break;
            }
            break;

          case ND_Equal:
          case ND_NotEqual:
            switch (lhs->type.kind) {
              case TYPE_Int:
                result =
                    ((ObjLong*)lhs)->value == ((ObjLong*)rhs)->value;
                break;

              case TYPE_Float:
                result = ((ObjFloat*)lhs)->value ==
                         ((ObjFloat*)rhs)->value;
                break;
            }

            if (item->kind == ND_NotEqual) result ^= 1;

            break;
        }

        if (!result) {
          return gcnew<ObjBool>(false);
        }

        lhs = rhs;
      }

      return gcnew<ObjBool>(true);
    }

    case ND_Assign: {
      auto& dest = this->eval_lvalue(node->nd_lhs);

      auto src = this->eval(node->nd_rhs);

      dest = src;

      return src;
    }
  }

  auto lhs = this->eval(node->nd_lhs);
  auto rhs = this->eval(node->nd_rhs);

  return this->compute_expr(node, lhs, rhs);
}
