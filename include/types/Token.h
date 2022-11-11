#pragma once

#include <string>
#include "types/Type.h"

enum TokenKind {
  TOK_Immediate,
  TOK_Ident,
  TOK_Keyword,
  TOK_Punctuator,
  TOK_End
};

struct Token {
  TokenKind kind;
  TypeKind imm_kind;
  Token* prev;
  Token* next;

  std::string_view str;

  size_t pos;
  size_t endpos;
  size_t linenum;

  Token(TokenKind kind)
      : kind(kind),
        imm_kind(TYPE_None),
        prev(nullptr),
        next(nullptr),
        pos(0),
        endpos(0),
        linenum(0)
  {
  }

  Token(TokenKind kind, Token* prev, size_t pos)
      : Token(kind)
  {
    this->prev = prev;
    this->pos = pos;

    this->prev->next = this;
  }
};
