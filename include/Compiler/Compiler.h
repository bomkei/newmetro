#pragma once

#include <vector>
#include "Asm.h"

struct Node;

class Compiler {
 public:
  Compiler();
  Compiler(Compiler&&) = delete;
  Compiler(Compiler const&) = delete;

  void compile_node(Node*, int8_t base = 3);

  void compile_expr(Node*, int8_t base = 3);
  void compile_stmt(Node*, int8_t base = 3);

  void compile_func(Node*);

  std::vector<Asm> const& get_compiled_codes();

 private:
  template <class... Args>
  Asm& push(Args&&... args)
  {
    return this->codes.emplace_back(args...);
  }

  std::vector<Asm> codes;
};
