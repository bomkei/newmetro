#include "types/Object.h"
#include "types/Node.h"
#include "types/Token.h"
#include "types/BuiltinFunc.h"
#include "Error.h"
#include "Utils.h"
#include "Parser.h"

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

        case TYPE_Float:
          node->nd_value =
              new ObjFloat(std::stof(this->cur->str.data()));

          break;

        case TYPE_String: {
          auto s =
              Utils::Converter::to_wide(std::string(this->cur->str));

          for (auto c = s.begin(); c != s.end(); c++) {
            if (*c == L'\\') {
              s.erase(c);

              switch (*c) {
                case L'\\':
                  break;

                case L'n':
                  *c = L'\n';
                  break;

                case L't':
                  *c = L'\t';
                  break;

                default:
                  TODO_IMPL
              }
            }
          }

          node->nd_value = new ObjString(std::move(s));
          break;
        }

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

      node->nd_variable_name = this->cur;
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

  //
  // for
  if (this->eat("for")) {
    auto node = new Node(ND_For, this->ate);

    node->nd_for_iterator = this->expr();

    this->expect("in");

    node->nd_for_range = this->expr();

    node->nd_for_loop_code = this->expect_scope();

    return node;
  }

  //
  // return
  if (this->eat("return")) {
    auto node = new Node(ND_Return, this->ate);

    if (!this->eat(";")) {
      node->nd_return_expr = this->expr();
      this->expect_semi();
    }

    return node;
  }

  //
  // break
  if (this->eat("break")) {
    auto node = new Node(ND_Break, this->ate);

    if (!this->eat(";")) {
      node->nd_break_expr = this->expr();
      this->expect_semi();
    }

    return node;
  }

  //
  // continue
  if (this->eat("continue")) {
    auto node = new Node(ND_Continue, this->ate);

    this->expect_semi();

    return node;
  }

  return this->factor();
}

Node* Parser::member_access()
{
  auto x = this->statement();

  while (this->check()) {
    // member access
    if (this->eat(".")) {
      // expect identifier
      if (this->cur->kind != TOK_Ident) {
        Error(ERR_InvalidSyntax, this->cur).emit().exit();
      }

      x = new Node(ND_MemberAccess, this->ate, x, this->statement());
    }

    // functor
    else if (this->eat("(")) {
      auto nd = new Node(ND_Callfunc, this->ate);

      nd->nd_callfunc_functor = x;

      if (!this->eat(")")) {
        do {
          nd->list.emplace_back(this->expr());
        } while (this->eat(","));

        this->expect(")");
      }

      if (x->kind == ND_MemberAccess) {
        nd->list.insert(nd->list.begin(), x->nd_lhs);
        nd->nd_callfunc_functor = x->nd_rhs;
      }

      x = nd;
    }

    // subscript
    else if (this->eat("[")) {
      x = new Node(ND_Subscript, this->ate, x, this->expr());
      this->expect("]");
    }

    // post inclement
    else if (this->eat("++")) {
      x = new Node(
          ND_Sub, this->ate,
          this->new_assign(ND_Add, this->ate, x,
                           this->new_value_nd(new ObjLong(1))),
          this->new_value_nd(new ObjLong(1)));
    }

    // post declement
    else if (this->eat("--")) {
      x = new Node(
          ND_Add, this->ate,
          this->new_assign(ND_Sub, this->ate, x,
                           this->new_value_nd(new ObjLong(1))),
          this->new_value_nd(new ObjLong(1)));
    }
    else {
      break;
    }
  }

  return x;
}

Node* Parser::unary()
{
  // unary declement
  if (this->eat("-")) {
    return new Node(ND_Sub, this->ate,
                    this->new_value_nd(new ObjLong(0)),
                    this->member_access());
  }

  // pre declement
  else if (this->eat("--")) {
    return this->new_assign(ND_Sub, this->ate, this->member_access(),
                            this->new_value_nd(new ObjLong(1)));
  }

  // pre inclement
  else if (this->eat("++")) {
    return this->new_assign(ND_Add, this->ate, this->member_access(),
                            this->new_value_nd(new ObjLong(1)));
  }

  this->eat("+");

  return this->member_access();
}

Node* Parser::mul()
{
  auto x = this->unary();

  while (this->check()) {
    if (this->eat("*"))
      x = new Node(ND_Mul, this->ate, x, this->unary());
    else if (this->eat("/"))
      x = new Node(ND_Div, this->ate, x, this->unary());
    else if (this->eat("%"))
      x = new Node(ND_Mod, this->ate, x, this->unary());
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

Node* Parser::shift()
{
  auto x = this->add();

  while (this->check()) {
    if (this->eat("<<"))
      x = new Node(ND_LShift, this->ate, x, this->add());
    else if (this->eat(">>"))
      x = new Node(ND_RShift, this->ate, x, this->add());
    else
      break;
  }

  return x;
}

Node* Parser::compare()
{
  auto x = this->shift();

  while (this->check()) {
    if (this->eat(">"))
      x = new Node(ND_Bigger, this->ate, x, this->shift());
    else if (this->eat("<"))
      x = new Node(ND_Bigger, this->ate, this->shift(), x);
    else if (this->eat(">="))
      x = new Node(ND_BiggerOrEqual, this->ate, x, this->shift());
    else if (this->eat("<="))
      x = new Node(ND_BiggerOrEqual, this->ate, this->shift(), x);
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

Node* Parser::bit_and()
{
  auto x = this->equality();

  while (this->eat("&"))
    x = new Node(ND_BitAnd, this->ate, x, this->equality());

  return x;
}

Node* Parser::bit_xor()
{
  auto x = this->bit_and();

  while (this->eat("^"))
    x = new Node(ND_BitXor, this->ate, x, this->bit_and());

  return x;
}

Node* Parser::bit_or()
{
  auto x = this->bit_xor();

  while (this->eat("|"))
    x = new Node(ND_BitOr, this->ate, x, this->bit_xor());

  return x;
}

Node* Parser::range()
{
  auto x = this->bit_or();

  if (this->eat(".."))
    return new Node(ND_Range, this->ate, x, this->bit_or());

  return x;
}

Node* Parser::log_and()
{
  auto x = this->range();

  while (this->eat("&&"))
    x = new Node(ND_LogAnd, this->ate, x, this->range());

  return x;
}

Node* Parser::log_or()
{
  auto x = this->log_and();

  while (this->eat("||"))
    x = new Node(ND_LogAnd, this->ate, x, this->log_and());

  return x;
}

Node* Parser::assign()
{
  auto x = this->log_or();

  if (this->eat("="))
    x = new Node(ND_Assign, this->ate, x, this->assign());

  if (this->eat("+="))
    x = this->new_assign(ND_Add, this->ate, x, this->assign());

  if (this->eat("-="))
    x = new Node(ND_Assign, this->ate, x,
                 new Node(ND_Sub, this->ate, x, this->assign()));

  if (this->eat("*="))
    x = new Node(ND_Assign, this->ate, x,
                 new Node(ND_Mul, this->ate, x, this->assign()));

  if (this->eat("/="))
    x = new Node(ND_Assign, this->ate, x,
                 new Node(ND_Div, this->ate, x, this->assign()));

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

bool Parser::check()
{
  return this->cur->kind != TOK_End;
}

void Parser::next()
{
  this->cur = this->cur->next;
}

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

bool Parser::eat_semi()
{
  return this->eat(";");
}

void Parser::expect_semi()
{
  if (!this->eat_semi())
    Error(ERR_InvalidSyntax, this->cur->prev)
        .suggest(this->cur->prev,
                 "expected semicolon after this token")
        .emit()
        .exit();
}

Node* Parser::new_value_nd(Object* obj)
{
  auto x = new Node(ND_Value);

  x->nd_value = obj;

  return x;
}

Node* Parser::new_assign(NodeKind kind, Token* token, Node* lhs,
                         Node* rhs)
{
  return new Node(ND_Assign, token, lhs,
                  new Node(kind, token, lhs, rhs));
}
