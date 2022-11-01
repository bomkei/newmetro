#include "metro.h"

#define nd_kind_expr_begin ND_Add

void Evaluator::adjust_object_type(Object*& lhs, Object*& rhs)
{
  if (static_cast<int>(lhs->type.kind) >
      static_cast<int>(rhs->type.kind)) {
    std::swap(lhs, rhs);
  }

  if (!lhs->type.equals(rhs->type)) {
    if (lhs->type.kind == TYPE_Int && rhs->type.kind == TYPE_Float) {
      lhs = new ObjFloat((float)((ObjLong*)lhs)->value);
    }
  }
}

Object* Evaluator::compute_expr(Node* node, Object* lhs, Object* rhs)
{
#define done goto finish
#define invalid goto __invalid_op

#define must(x, T) \
  if (!x.equals(T)) Error(ERR_TypeMismatch, node).emit().exit()

  static constexpr void* jump_table[] = {
      &&expr_add,
      &&expr_sub,
      &&expr_mul,
      &&expr_div,
      &&expr_mod,

      // shift
      &&expr_lshift,
      &&expr_rshift,

      // compare
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,

      // bit
      &&expr_bit_and,
      &&expr_bit_xor,
      &&expr_bit_or,

      // log
      &&expr_log_and,
      &&expr_log_or,
  };

  static std::tuple<TypeKind, TypeKind, void*> const
      jump_table_special[]{{TYPE_Int, TYPE_String, &&mul_int_str}};

  this->adjust_object_type(lhs, rhs);

  Object* result = lhs;
  auto typekind = lhs->type.kind;

  if (lhs->type.equals(rhs->type)) {
    goto*
        jump_table[static_cast<int>(node->kind) - nd_kind_expr_begin];
  }

  for (auto&& [left, right, label] : jump_table_special) {
    if (lhs->type.equals(left) && rhs->type.equals(right))
      goto* label;
  }

  invalid;

expr_add:
  switch (typekind) {
    case TYPE_Int:
      ((ObjLong*)result)->value += ((ObjLong*)rhs)->value;
      done;

    case TYPE_Float:
      ((ObjFloat*)result)->value += ((ObjFloat*)rhs)->value;
      done;
  }
  invalid;

expr_sub:
  switch (typekind) {
    case TYPE_Int:
      ((ObjLong*)result)->value -= ((ObjLong*)rhs)->value;
      done;

    case TYPE_Float:
      ((ObjFloat*)result)->value -= ((ObjFloat*)rhs)->value;
      done;
  }
  invalid;

expr_mul:
  switch (typekind) {
    case TYPE_Int:
      ((ObjLong*)result)->value *= ((ObjLong*)rhs)->value;
      done;

    case TYPE_Float:
      ((ObjFloat*)result)->value *= ((ObjFloat*)rhs)->value;
      done;
  }
  invalid;

expr_div:
  switch (typekind) {
    case TYPE_Int:
      ((ObjLong*)result)->value /= ((ObjLong*)rhs)->value;
      done;

    case TYPE_Float:
      ((ObjFloat*)result)->value /= ((ObjFloat*)rhs)->value;
      done;
  }
  invalid;

expr_mod:
  switch (typekind) {
    case TYPE_Int:
      ((ObjLong*)result)->value %= ((ObjLong*)rhs)->value;
      done;

    case TYPE_Float: {
      auto& a = ((ObjFloat*)result)->value;
      auto b = ((ObjFloat*)rhs)->value;

      while (a >= b) {
        a -= b;
      }

      done;
    }
  }
  invalid;

expr_lshift:
  must(lhs->type, TYPE_Int);
  ((ObjLong*)result)->value <<= ((ObjLong*)rhs)->value;
  done;

expr_rshift:
  must(lhs->type, TYPE_Int);
  ((ObjLong*)result)->value >>= ((ObjLong*)rhs)->value;
  done;

expr_bit_and:
  must(lhs->type, TYPE_Int);
  ((ObjLong*)result)->value &= ((ObjLong*)rhs)->value;
  done;

expr_bit_xor:
  must(lhs->type, TYPE_Int);
  ((ObjLong*)result)->value ^= ((ObjLong*)rhs)->value;
  done;

expr_bit_or:
  must(lhs->type, TYPE_Int);
  ((ObjLong*)result)->value |= ((ObjLong*)rhs)->value;
  done;

expr_log_and:
  must(lhs->type, TYPE_Bool);
  must(rhs->type, TYPE_Bool);
  ((ObjBool*)result)->value &= ((ObjBool*)rhs)->value;
  done;

expr_log_or:
  must(lhs->type, TYPE_Bool);
  must(rhs->type, TYPE_Bool);
  ((ObjBool*)result)->value |= ((ObjBool*)rhs)->value;
  done;

mul_int_str : {
  auto count = ((ObjLong*)lhs)->value;
  result = rhs->clone();

  if (count < 0)
    Error(ERR_MultiplyStringByNegative, node->nd_lhs).emit().exit();

  for (int64_t i = 1; i < count; i++) {
    ((ObjString*)result)->value += ((ObjString*)rhs)->value;
  }

  done;
}

finish:;
  return result;

__invalid_op:
  Error(ERR_InvalidOperator, node->token).emit().exit();
}
