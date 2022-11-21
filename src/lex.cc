#include "types/Node.h"
#include "types/Token.h"
#include "types/Source.h"
#include "Error.h"
#include "Utils.h"
#include "Lexer.h"

static char const punctuators[] =
    "(){}[]<>"  // brackets
    "=+-*/%"    // assign, expr
    "|^&"       // bit expr
    "@"         // self call
    ".,;:"
    "!?";

static char const* long_punctuators[]{
    // argument pack
    "...",

    // spaceship
    "<=>",

    // shift assign
    "<<=",
    ">>=",

    // range
    "..",

    // function return type specifier
    "->",

    // post/pre incl, decl
    "++",
    "--",

    // composite assign
    "+=",
    "-=",
    "*=",
    "/=",
    "%=",
    "&=",
    "^=",
    "|=",

    // shift
    ">>",
    "<<",

    // compare
    ">=",
    "<=",
    "==",
    "!=",

    //
    "&&",
    "||",
};

static std::string_view const keywords[]{
    // immediate value
    "true",
    "false",

    // type name
    "none",
    "int",
    "float",
    "bool",
    "char",
    "string",
    "tuple",
    "vector",
    "funcion",

    // control expr
    "if",
    "else",
    "switch",
    "match",
    "for",
    "loop",
    "while",
    "do",
    "break",
    "continue",
    "return",

    // variable declaration
    "let",

    // function
    "fn",
    "self",

    // global
    "class",
    "namespace",
};

Lexer::Lexer(Source& source)
    : source(source),
      position(0),
      length(source.text.length())
{
  this->initialize();
}

Token* Lexer::lex()
{
  Token top{TOK_End};
  Token* cur = &top;

  auto line_itr = this->line_list.cbegin();

  this->pass_space();

  while (this->check()) {
    auto ch = this->peek();
    auto pos = this->position;

    auto str = this->get_raw_ptr();
    size_t len = 0;

    cur = new Token(TOK_Immediate, cur, pos);

    // digits
    if (isdigit(ch)) {
      cur->kind = TOK_Immediate;
      cur->imm_kind = TYPE_Int;

      len = this->pass_while(isalnum);

      if (this->peek() == '.') {
        this->position++;

        if (!isdigit(this->peek())) {
          this->position--;
        }
        else {
          cur->imm_kind = TYPE_Float;
          len += this->pass_while(isalnum) + 1;
        }
      }
    }

    // char / string
    else if (ch == '"' || ch == '\'') {
      cur->kind = TOK_Immediate;
      cur->imm_kind = ch == '"' ? TYPE_String : TYPE_Char;

      this->position++;
      str++;

      len = this->pass_while([&](char c) { return c != ch; });

      this->position++;
    }

    // identifier
    else if (isalpha(ch) || ch == '_') {
      cur->kind = TOK_Ident;
      len = this->pass_while(
          [](char c) { return isalnum(c) || c == '_'; });
    }

    // punctuator
    else {
      cur->kind = TOK_Punctuator;

      for (auto&& pu : long_punctuators) {
        if ((len = this->match(pu)) != -1) {
          str = pu;
          this->position += len;

          goto _found;
        }
      }

      if (auto r = std::find(punctuators, std::end(punctuators),
                             this->peek());
          r != std::end(punctuators)) {
        cur->kind = TOK_Punctuator;
        str = r;
        len = 1;
        this->position++;
        goto _found;
      }

      Error(ERR_InvalidToken, pos).emit().exit();

    _found:;
    }

    cur->str = {str, len};
    cur->endpos = this->position;

    /*
    if (cur->kind == TOK_Ident &&
        std::find(std::begin(keywords), std::end(keywords),
                  cur->str) != std::end(keywords)) {
      cur->kind = TOK_Keyword;
    }
    */

    while (line_itr->second < this->position) {
      line_itr++;
    }

    cur->linenum = line_itr - this->line_list.begin() + 1;

    this->pass_space();
  }

  cur = new Token(TOK_End, cur, this->position);

  return top.next;
}

void Lexer::initialize()
{
  size_t j = 0;

  for (size_t i = 0; i < this->length; i++) {
    if (this->source.text[i] == '\n') {
      this->line_list.emplace_back(j, i);
      j = i;
    }
  }
}

bool Lexer::check()
{
  return this->position < this->length;
}

char Lexer::peek()
{
  return this->source.text[this->position];
}

int Lexer::match(std::string_view s)
{
  if (this->position + s.length() <= this->length &&
      memcmp(this->get_raw_ptr(), s.data(), s.length()) == 0)
    return s.length();

  return -1;
}

void Lexer::pass_space()
{
  while (isspace(this->peek())) {
    this->position++;
  }
}

size_t Lexer::pass_while(std::function<bool(char)> cond)
{
  size_t count{};

  while (cond(this->peek())) {
    count++;
    this->position++;
  }

  return count;
}

char const* Lexer::get_raw_ptr()
{
  return this->source.text.data() + this->position;
}