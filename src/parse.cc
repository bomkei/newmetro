#include "metro.h"

Parser::Parser(Token* token)
    : cur(token),
      ate(nullptr)
{
}

Node* Parser::atom()
{
  if (this->eat("@")) {
    return new Node(ND_SelfFunc, this->ate);
  }

  if (this->eat("true")) {
    return new Node(ND_True, this->ate);
  }

  if (this->eat("false")) {
    return new Node(ND_False, this->ate);
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

        case TYPE_String:
          node->nd_value = new ObjString(
              Utils::Converter::to_wide(std::string(this->cur->str)));

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

Node* Parser::factor()
{
  auto token = this->cur;

  // スコープ
  if (this->cur->str == "{") {
    return this->expect_scope();
  }

  // 括弧
  if (this->eat("(")) {
    auto x = this->expr();

    // カンマがあったらタプル
    if (this->eat(",")) {
      x = Node::new_list(ND_Tuple, token, x);

      do {
        x->append(this->expr());
      } while (this->eat(","));
    }

    this->expect(")");

    return x;
  }

  // リスト
  if (this->eat("[")) {
    if (this->eat("]")) {  // 要素なし
      return new Node(ND_EmptyList, token);
    }

    auto node = new Node(ND_List, token);

    do {
      node->append(this->expr());
    } while (this->eat(","));

    this->expect("]");

    return node;
  }

  return this->atom();
}

//
// ステートメント
Node* Parser::statement()
{
  //
  // function
  if (this->eat("fn")) {
    auto node = new Node(ND_Function, this->ate);

    this->expect("(");

    if (!this->eat(")")) {
      do {
        auto& arg =
            node->list.emplace_back(new Node(ND_Argument, this->cur));

        arg->nd_arg_name = this->expect_ident();

        if (this->eat(":")) {
          arg->nd_arg_type = this->expect_type();
        }

      } while (this->eat(","));

      this->expect(")");
    }

    if (this->eat("->")) {
      node->nd_func_return_type = this->expect_type();
    }

    node->nd_func_code = this->expect_scope();

    return node;
  }

  //
  // if
  if (this->eat("if")) {
    auto node = new Node(ND_If, this->ate);

    node->nd_if_cond = this->expr();

    node->nd_if_true = this->expect_scope();

    if (this->eat("else")) {
      if (this->cur->str == "if")
        node->nd_if_false = this->expr();
      else
        node->nd_if_false = this->expect_scope();
    }

    return node;
  }

  return this->factor();
}

Node* Parser::callfunc()
{
  auto x = this->statement();

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

Node* Parser::subscript()
{
  auto x = this->callfunc();

  while (this->eat("[")) {
    x = new Node(ND_Subscript, this->ate, x, this->expr());

    this->expect("]");
  }

  return x;
}

Node* Parser::member_access()
{
  auto x = this->subscript();

  while (this->eat(".")) {
    if (this->cur->kind != TOK_Ident) {
      Error(ERR_InvalidSyntax, this->cur).emit().exit();
    }

    auto y = this->subscript();

    if (y->kind == ND_Callfunc) {
      y->list.insert(y->list.begin(), x);
      x = y;
    }
    else {
      x = new Node(ND_MemberAccess, this->ate, x, this->subscript());
    }
  }

  return x;
}

Node* Parser::mul()
{
  auto x = this->member_access();

  while (this->check()) {
    if (this->eat("*"))
      x = new Node(ND_Mul, this->ate, x, this->member_access());
    else if (this->eat("/"))
      x = new Node(ND_Div, this->ate, x, this->member_access());
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

Node* Parser::compare()
{
  auto x = this->add();

  while (this->check()) {
    if (this->eat(">"))
      x = new Node(ND_Bigger, this->ate, x, this->add());
    else if (this->eat("<"))
      x = new Node(ND_Bigger, this->ate, this->add(), x);
    else if (this->eat(">="))
      x = new Node(ND_BiggerOrEqual, this->ate, x, this->add());
    else if (this->eat("<="))
      x = new Node(ND_BiggerOrEqual, this->ate, this->add(), x);
    else
      break;
  }

  return x;
}

Node* Parser::equality()
{
  auto x = this->compare();

  while (this->check()) {
    if (this->eat("=="))
      x = new Node(ND_Equal, this->ate, x, this->compare());
    else if (this->eat("!="))
      x = new Node(ND_NotEqual, this->ate, x, this->compare());
    else
      break;
  }

  return x;
}

Node* Parser::assign()
{
  auto x = this->equality();

  if (this->eat("="))
    x = new Node(ND_Assign, this->ate, x, this->assign());

  if (this->eat("+="))
    x = new Node(ND_Assign, this->ate, x,
                 new Node(ND_Add, this->ate, x, this->assign()));

  return x;
}

Node* Parser::expr()
{
  //
  // let - 変数定義
  if (this->eat("let")) {
    auto node = new Node(ND_Let, this->ate);

    // 変数名
    node->nd_let_name = this->expect_ident();

    if (this->eat(":")) {  // 型指定
      node->nd_let_type = this->expect_type();
    }

    if (this->eat("=")) {  // 初期化式
      node->nd_let_init = this->expr();
    }

    this->expect(";");

    return node;
  }

  return this->assign();
}

Node* Parser::parse()
{
  auto node = new Node(ND_Scope);

  while (this->check()) {
    auto& item = node->list.emplace_back(this->expr());

    if (this->cur->prev->str == "}") {
      continue;
    }

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

    alert;
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
