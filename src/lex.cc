#include "metro.h"

static char const punctuators[] =
    "(){}[]<>"  // 括弧
    "=+-*/%"    // 代入、基本演算
    "|^&"       // ビット演算
    "@"         // 関数再帰 (自分自身の呼び出し)
    ".,;:"
    "!?";

static char const* long_punctuators[]{
    // 三方比較演算子
    "<=>",

    // シフト代入
    "<<=",
    ">>=",

    // 関数の戻り値の型を指定
    "->",

    // 複合代入
    "+=",
    "-=",
    "*=",
    "/=",
    "%=",
    "&=",
    "^=",
    "|=",

    // シフト
    ">>",
    "<<",

    // 比較
    ">=",
    "<=",
    "==",
    "!=",

    //
    "&&",
    "||",
};

static std::string_view const keywords[]{
    // 即値
    "true",
    "false",

    // 型名
    "none",
    "int",
    "float",
    "bool",
    "char",
    "string",
    "tuple",
    "vec",
    "func",

    // 制御構文
    "if",
    "else",
    "for",
    "switch",
    "match",
    "loop",
    "while",
    "do",
    "break",
    "continue",
    "return",

    // 変数定義
    "let",

    // 関数
    "fn",
    // "self",

    // クラス
    "class",

    // 名前空間
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

    if (cur->kind == TOK_Ident &&
        std::find(std::begin(keywords), std::end(keywords),
                  cur->str) != std::end(keywords)) {
      cur->kind = TOK_Keyword;
    }

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