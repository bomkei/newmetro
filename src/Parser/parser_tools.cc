#include "types/Object.h"
#include "types/Node.h"
#include "types/Token.h"
#include "types/BuiltinFunc.h"
#include "Error.h"
#include "Utils.h"
#include "Parser.h"

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

  // todo: parse template parameters < ... >

  // todo: parse mutable
  // todo: parase reference

  return node;
}

Node* Parser::expect_scope(bool is_func_scope,
                           std::function<Node*(Parser*)> chi)
{
  /*
  static auto new_return = [&](Node* x) -> Node* {
    if (!is_func_scope)
      return x;

    if (x->kind != ND_Return) {
      auto y = new Node(ND_Return, x->token);

      y->nd_return_expr = x;
      x = y;
    }

    return x;
  };
  */

  auto node = new Node(ND_Scope, this->cur);

  this->expect("{");

  // empty scope
  if (this->eat("}")) {
    return node;
  }

  while (this->check()) {
    auto& item = node->list.emplace_back(chi(this));

    if (auto semi = this->cur;
        this->eat(";") || (semi = this->cur->prev)->str == ";") {
      if (this->eat("}")) {
        node->list.emplace_back(new Node(ND_None, semi));
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

//
// ------------------------------------------------
//  ノードを return 文に変換する
//  関数スコープ内の最後の要素にだけ使ってください
// ------------------------------------------------
Node* Parser::to_return_stmt(Node* node)
{
  switch (node->kind) {
    case ND_Return:
      return node;

    //
    // if 文
    case ND_If: {
      //
      // else がある場合は、if 文で戻り値が確定するか確認する
      if (auto nd_else = node->nd_if_false; nd_else) {
        while (nd_else->kind == ND_If && nd_else->nd_if_false) {
          nd_else = nd_else->nd_if_false;
        }

        //
        // 最後が else if で終わっていればエラー
        if (nd_else && nd_else->kind == ND_If) {
          Error(ERR_MayNotToBeEvaluated, node->token)
              .suggest(nd_else,
                       "the condition of if-statement must evalaute "
                       "to true")
              .emit()
              .exit();
        }
      }

      break;
    }
  }

  auto nd_ret = new Node(ND_Return, node->token);

  nd_ret->nd_return_expr = node;

  return nd_ret;
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
