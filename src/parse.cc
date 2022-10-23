#include "metro.h"

Parser::Parser(Token* token)
    : cur(token),
      ate(nullptr)
{
}

Node* Parser::factor()
{
  if (this->eat("{")) {
    TODO_IMPL
  }

  //
  // 変数定義
  if (this->eat("let")) {
    auto node = new Node(ND_Let, this->ate);

    node->nd_let_name = this->expect_ident();

    if (this->eat(":")) {
      node->nd_let_type = this->expect_type();
    }

    if (this->eat("=")) {
      node->nd_let_init = this->expr();
    }

    this->expect(";");

    return node;
  }

  switch (this->cur->kind) {
    //
    // 即値
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

    //
    // 変数
    case TOK_Ident: {
      auto node = new Node(ND_Variable, this->cur);

      this->next();

      return node;
    }
  }

  Error(ERR_InvalidSyntax, this->cur).emit().exit();
}

Node* Parser::callfunc()
{
  auto x = this->factor();

  if (this->eat("(")) {
    auto nd = new Node(ND_Callfunc, this->ate);

    nd->nd_callfunc_functor = x;

    if (!this->eat(")")) {
      do {
        nd->list.emplace_back(this->expr());
      } while (this->eat(","));

      this->expect(")");
    }

    return nd;
  }

  return x;
}

Node* Parser::mul()
{
  auto x = this->callfunc();

  while (this->check()) {
    if (this->eat("*"))
      x = new Node(ND_Mul, this->ate, x, this->callfunc());
    else if (this->eat("/"))
      x = new Node(ND_Div, this->ate, x, this->callfunc());
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

Node* Parser::function()
{
  if (this->eat("fn")) {
    auto node = new Node(ND_Function, this->ate);

    node->nd_func_name = this->expect_ident();

    this->expect("(");

    if (!this->eat(")")) {
      do {
        auto& arg = node->list.emplace_back(new Node(ND_Argument));

        arg->nd_arg_name = this->expect_ident();

        this->expect(":");

        arg->nd_arg_type = this->expect_type();

      } while (this->eat(","));

      this->expect(")");
    }

    this->expect("->");

    node->nd_func_return_type = this->expect_type();

    node->nd_func_code = this->expect_scope();

    return node;
  }

  return this->expr();
}

Node* Parser::parse()
{
  auto node = new Node(ND_Scope);

  while (this->check()) {
    node->list.emplace_back(this->function());

    if (this->cur->prev->str == ";" || this->eat(";")) {
      if (this->check()) {
        continue;
      }

      node->list.emplace_back(new Node(ND_None));
      break;
    }
    else if (this->cur->kind == TOK_End) {
      break;
    }

    Error(ERR_UnexpectedToken, this->cur->prev)
        .suggest(this->cur->prev,
                 "expected semicolon after this token")
        .emit()
        .exit();
  }

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
    Error(ERR_UnexpectedToken, this->cur).emit().exit();
  }
}

Token* Parser::expect_ident(bool allow_kwd)
{
  if (this->cur->kind == TOK_Ident ||
      (allow_kwd && this->cur->kind == TOK_Keyword)) {
    this->ate = this->cur;
    this->next();

    return this->ate;
  }

  Error(ERR_ExpectedIdentifier, this->cur).emit().exit();
}

Node* Parser::expect_type()
{
  auto node = new Node(ND_Type, this->expect_ident(true));

  // todo: < ... >
  // todo: check mutable
  // todo: check reference

  return node;
}

Node* Parser::expect_scope()
{
  auto node = new Node(ND_Scope, this->cur);

  this->expect("{");

  // empty scope
  if (this->eat("}")) {
    return node;
  }

  while (this->check()) {
    auto& item = node->list.emplace_back(this->expr());

    if (this->eat(";") || this->cur->prev->str == ";") {
      if (this->eat("}")) {
        node->list.emplace_back(new Node(ND_None));
        return node;
      }

      continue;
    }

    if (this->eat("}")) {
      return node;
    }

    if (this->cur->prev->str != "}") {
      this->expect("}");
    }
  }

  Error(ERR_BracketNotClosed, node->token).emit().exit();
}
