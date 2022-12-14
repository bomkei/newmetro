#include "types/Object.h"
#include "types/Node.h"
#include "types/Token.h"
#include "types/BuiltinFunc.h"
#include "Error.h"
#include "Utils.h"
#include "Evaluator.h"

#define nd_kind_expr_begin ND_Add

template <class T> bool is_overflow(NodeKind kind, T a, T b) {
  switch (kind) {
  case ND_Add:
    return (a > 0 && b > 0 && a > std::numeric_limits<T>::max() - b) ||
           (a < 0 && b < 0 && a < std::numeric_limits<T>::min() - b);
  case ND_Sub:
    return (a > 0 && b < 0 && a > std::numeric_limits<T>::max() + b) ||
           (a < 0 && b > 0 && a < std::numeric_limits<T>::min() + b);
  case ND_Mul:
    return (a > 0 && b > 0 && a > std::numeric_limits<T>::max() / b) ||
           (a > 0 && b < 0 && b < std::numeric_limits<T>::min() / a) ||
           (a < 0 && b > 0 && a < std::numeric_limits<T>::min() / b) ||
           (a < 0 && b < 0 && b < std::numeric_limits<T>::max() / a);
  default:
    return false;
  }
}

Object* Evaluator::compute_expr(Node* node, Object* lhs, Object* rhs)
{
#define done goto finish
#define invalid goto __invalid_op

#define must(x, T) \
  if (!x.equals(T)) Error(ERR_TypeMismatch, node).emit().exit()

#define check_overflow(kind, a, b) \
  if (is_overflow(kind, a, b)) Error(ERR_ValueOutOfRange, node).emit().exit()

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

  Object* result = lhs->clone();
  auto typekind = lhs->type.kind;

  if (lhs->type.equals(rhs->type)) {
    goto* jump_table[node->kind - nd_kind_expr_begin];
  }

  for (auto&& [left, right, label] : jump_table_special) {
    if (lhs->type.equals(left) && rhs->type.equals(right))
      goto* label;
  }

  invalid;

expr_add:
  switch (typekind) {
    case TYPE_Int:
      check_overflow(ND_Add, ((ObjLong*)lhs)->value, ((ObjLong*)rhs)->value);
      ((ObjLong*)result)->value += ((ObjLong*)rhs)->value;
      done;

    case TYPE_Float:
      check_overflow(ND_Add, ((ObjFloat*)lhs)->value, ((ObjFloat*)rhs)->value);
      ((ObjFloat*)result)->value += ((ObjFloat*)rhs)->value;
      done;

    case TYPE_String:
      check_overflow(ND_Add, ((ObjString*)lhs)->value.size(),
                     ((ObjString*)rhs)->value.size());
      ((ObjString*)result)->value += ((ObjString*)rhs)->value;
      done;
  }
  invalid;

expr_sub:
  switch (typekind) {
    case TYPE_Int:
      check_overflow(ND_Sub, ((ObjLong*)lhs)->value, ((ObjLong*)rhs)->value);
      ((ObjLong*)result)->value -= ((ObjLong*)rhs)->value;
      done;

    case TYPE_Float:
      check_overflow(ND_Sub, ((ObjFloat*)lhs)->value, ((ObjFloat*)rhs)->value);
      ((ObjFloat*)result)->value -= ((ObjFloat*)rhs)->value;
      done;
  }
  invalid;

expr_mul:
  switch (typekind) {
    case TYPE_Int:
      check_overflow(ND_Mul, ((ObjLong*)lhs)->value, ((ObjLong*)rhs)->value);
      ((ObjLong*)result)->value *= ((ObjLong*)rhs)->value;
      done;

    case TYPE_Float:
      check_overflow(ND_Mul, ((ObjFloat*)lhs)->value, ((ObjFloat*)rhs)->value);
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

Object*& Evaluator::compute_subscript(Node* node, Object* lhs,
                                      Object* index)
{
  if (!lhs->type.equals(TYPE_Vector)) {
    Error(ERR_TypeMismatch, node->nd_lhs)
        .suggest(node->nd_lhs,
                 "expected `vector` or `tuple`, but found `" +
                     lhs->type.to_string() + "`")
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
