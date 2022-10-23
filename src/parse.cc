#include "metro.h"

Parser::Parser(Token* token)
    : cur(token),
      ate(nullptr)
{
}

Node* Parser::factor()
{
  switch (this->cur->kind) {
    case TOK_Immediate: {
      break;
    }
  }
}

Node* Parser::mul() {}

Node* Parser::add() {}

Node* Parser::expr() {}

Node* Parser::parse() {}

bool Parser::check() { return this->cur->kind != TOK_End; }

void Parser::next() { this->cur = this->cur->next; }

bool Parser::eat(std::string_view s)
{
  if (this->cur->str == s) {
    this->ate = this->cur;
    this->next();
    return true;
  }

  return false;
}

void Parser::expect(std::string_view s)
{
  if (!this->eat(s)) {
    Error(ERR_UnexpectedToken, this->cur)
        .suggest(this->cur, "expected identifier")
        .emit()
        .exit();
  }
}

Token* Parser::expect_ident()
{
  if (this->cur->kind == TOK_Ident) {
    this->ate = this->cur;
    this->next();

    return this->ate;
  }

  Error(ERR_ExpectedIdentifier, this->cur).emit().exit();
}