#include "types/Node.h"
#include "types/Object.h"
#include "Compiler/Compiler.h"
#include "Utils.h"

Compiler::Compiler()
{
}

void Compiler::compile_node(Node* node, int8_t base)
{
  this->compile_expr(node, base);
}

void Compiler::compile_expr(Node* node, int8_t base)
{
  if (!node)
    return;

  switch (node->kind) {
    case ND_Scope:
    case ND_Return:
    case ND_If:
      this->compile_stmt(node);
      break;

    case ND_Value: {
      this->push(ASM_Loadi, base, base).object = node->nd_value;
      break;
    }

    default: {
      this->compile_expr(node->nd_lhs, base);
      this->compile_expr(node->nd_rhs, base + 1);

      switch (node->kind) {
        case ND_Add:
          this->push(ASM_Add, base, base + 1);
          break;

        case ND_Sub:
          this->push(ASM_Sub, base, base + 1);
          break;
      }

      break;
    }
  }
}

void Compiler::compile_stmt(Node* node, int8_t base)
{
  if (!node)
    return;

  switch (node->kind) {
    case ND_Scope: {
      for (auto&& nd : node->list) {
        this->compile_node(nd);
      }

      break;
    }
  }
}

void Compiler::compile_func(Node* node)
{
}

std::vector<Asm> const& Compiler::get_compiled_codes()
{
  return this->codes;
}
