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
      auto node = new Node(ND_Value, this->cur);

      switch (this->cur->imm_kind) {
        case TYPE_Int:
          node->nd_value =
              new ObjLong(std::stoi(this->cur->str.data()));

          break;

        default:
          TODO_IMPL
      }

      this->next();

      return node;
    }

    case TOK_Ident: {
      break;
    }
  }

  Error(ERR_InvalidSyntax, this->cur).emit().exit();
}

Node* Parser::mul()
{
  auto x = this->factor();

  while (this->check()) {
    if (this->eat("*"))
      x = new Node(ND_Mul, this->ate, x, this->factor());
    else if (this->eat("/"))
      x = new Node(ND_Div, this->ate, x, this->factor());
    else
      break;
  }

  return x;
}

Node* Parser::add()
{
  auto x = this->mul();

  while (this->check()) {
    if (this->eat("+"))
      x = new Node(ND_Add, this->ate, x, this->mul());
    else if (this->eat("-"))
      x = new Node(ND_Sub, this->ate, x, this->mul());
    else
      break;
  }

  return x;
}

Node* Parser::expr() { return this->add(); }

Node* Parser::parse()
{
  auto node = this->expr();

  return node;
}

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