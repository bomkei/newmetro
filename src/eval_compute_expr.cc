#include "metro.h"

#define nd_kind_expr_begin ND_Add

Object* Evaluator::compute_expr(Node* node, Object* lhs, Object* rhs)
{
#define done goto finish
#define invalid goto __invalid_op

  static constexpr void* jump_table[] = {
      &&expr_add,
      &&expr_sub,
  };

  auto result = lhs->clone();
  auto typekind = lhs->type.kind;

  if (!lhs->type.equals(rhs->type)) {
    if (lhs->type.kind == TYPE_Float && rhs->type.kind == TYPE_Int) {
      rhs = new ObjFloat((float)((ObjLong*)rhs)->value);
    }
    else if (lhs->type.kind == TYPE_Int &&
             rhs->type.kind == TYPE_Float) {
      lhs = new ObjFloat((float)((ObjLong*)lhs)->value);
    }
  }

  goto* jump_table[static_cast<int>(node->kind) - nd_kind_expr_begin];

expr_add:;
  switch (typekind) {
    case TYPE_Int:
      ((ObjLong*)result)->value += ((ObjLong*)rhs)->value;
      done;

    case TYPE_Float:
      ((ObjFloat*)result)->value += ((ObjFloat*)rhs)->value;
      done;

    case TYPE_Vector:
      ((ObjVector*)result)->list.emplace_back(rhs);
      done;
  }

  invalid;

expr_sub:;
  switch (typekind) {
    case TYPE_Int:
      ((ObjLong*)result)->value -= ((ObjLong*)rhs)->value;
      done;

    case TYPE_Float:
      ((ObjFloat*)result)->value -= ((ObjFloat*)rhs)->value;
      done;
  }

  invalid;

finish:;
  return result;

__invalid_op:
  Error(ERR_InvalidOperator, node->token).emit().exit();
}
