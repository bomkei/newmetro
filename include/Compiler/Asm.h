#pragma once

enum AsmKind {
  ASM_Mov,
  ASM_Loadi,

  ASM_Add,
  ASM_Sub,
  ASM_Mul,
  ASM_Div,

  ASM_Jump,
  ASM_Call,

  ASM_Define,
  ASM_Label,
};

struct Asm {};