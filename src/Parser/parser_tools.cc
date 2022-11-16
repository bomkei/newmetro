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

Node* Parser::expect_scope(std::function<Node*(Parser*)> chi)
{
  auto node = new Node(ND_Scope, this->cur);

  this->expect("{");

  // empty scope
  if (this->eat("}")) {
    return node;
  }

  while (this->check()) {
    auto& item = node->list.emplace_back(chi(this));

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
