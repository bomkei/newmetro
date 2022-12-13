#include <string>
#include "Compiler/Asm.h"
#include "types/Object.h"
#include "Utils.h"

Asm::Asm(AsmKind kind)
    : kind(kind),
      reg_dest(0),
      reg_src(0),
      jump_to(0),
      object(nullptr)
{
}

Asm::Asm(AsmKind kind, int8_t reg_dest, int8_t reg_src)
    : kind(kind),
      reg_dest(reg_dest),
      reg_src(reg_src),
      jump_to(0),
      object(nullptr)
{
}

std::string Asm::to_string() const
{
  switch (this->kind) {
    case ASM_Mov:
      return Utils::format("mov r%hhd, r%hhd", this->reg_dest,
                           this->reg_src);

    case ASM_Loadi:
      return Utils::format("loadi r%hhd, #%s", this->reg_dest,
                           this->object->to_string().c_str());

    case ASM_Add:
      return Utils::format("add r%hhd, r%hhd", this->reg_dest,
                           this->reg_src);

    case ASM_Sub:
      return Utils::format("sub r%hhd, r%hhd", this->reg_dest,
                           this->reg_src);

    default:
      return "??";
  }

  return Utils::format("");
}
