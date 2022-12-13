#pragma once

#include "Asm.h"

struct Node;

class Compiler {
 public:
  Compiler();
  Compiler(Compiler&&) = delete;
  Compiler(Compiler const&) = delete;

  void compile_node(Node*);

  void compile_expr(Node*);
  void compile_stmt(Node*);

  void compile_func(Node*);

 private:
};
