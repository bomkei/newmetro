#include "metro.h"

static char const punctuators[] =
    "(){}[]<>"
    "+=*/%"
    "|^&"
    ".,"
    "!?";

static char const* long_punctuators[]{
    "<=>", "<<=", ">>=", "+=", "-=", "*=", "/=",
    "%=",  ">>",  "<<",  ">=", "<=", "==", "!=",
};

Lexer::Lexer(Source& source)
    : position(0),
      length(source.text.length()),
      source(source)
{
}

Token* Lexer::lex()
{
  Token top{TOK_End};
  Token* cur = &top;

  this->pass_space();

  while (this->check()) {
    auto ch = this->peek();
    auto pos = this->position;

    auto str = this->get_raw_ptr();
    size_t len = 0;
    TokenKind kind;

    // digits
    if (isdigit(ch)) {
      kind = TOK_Immediate;
      len = this->pass_while(isalnum);
    }

    // identifier
    else if (isalpha(ch) || ch == '_') {
      kind = TOK_Ident;
      len = this->pass_while(
          [](char c) { return isalnum(c) || c == '_'; });
    }

    // punctuator
    else if (auto r = std::find(punctuators, std::end(punctuators),
                                this->peek());
             r != std::end(punctuators)) {
      kind = TOK_Punctuator;
      str = r;
      len = 1;
      this->position++;
    }

    // long punctuator
    else {
      kind = TOK_Punctuator;

      for (auto&& pu : long_punctuators) {
        if ((len = this->match(pu)) != -1) {
          str = pu;
          this->position += len;

          goto _found;
        }
      }

      Error(ERR_InvalidToken, pos).emit().exit();

    _found:;
    }

    cur = new Token(kind, cur, pos);
    cur->str = {str, len};
    cur->endpos = this->position - pos;

    this->pass_space();
  }

  cur = new Token(TOK_End, cur, this->position);

  return top.next;
}

bool Lexer::check() { return this->position < this->length; }

char Lexer::peek() { return this->source.text[this->position]; }

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