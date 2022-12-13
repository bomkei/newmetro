#pragma once

enum AsmKind {
  ASM_Mov,
  ASM_Loadi,

  ASM_Cmp,

  ASM_Add,
  ASM_Sub,
  ASM_Mul,
  ASM_Div,

  ASM_Jump,
  ASM_Call,

  ASM_Define,
  ASM_Label,
};

struct Object;

struct Asm {
  AsmKind kind;

  int8_t reg_dest;
  int8_t reg_src;

  size_t jump_to;

  Object* object;

  Asm(AsmKind kind);
  Asm(AsmKind kind, int8_t reg_dest, int8_t reg_src);

  std::string to_string() const;
};