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

Node* Parser::function()
{
  //
  // function
  if (this->eat("fn")) {
    auto node = new Node(ND_Function, this->ate);

    node->nd_func_name = this->expect_ident();

    this->expect("(");

    if (!this->eat(")")) {
      do {
        auto& arg =
            node->list.emplace_back(new Node(ND_Argument, this->cur));

        if (this->eat("...")) {
          arg->kind = ND_VariableArguments;
          arg->nd_arg_name = this->expect_ident();
          break;
        }

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

  return this->expr();
}

Node* Parser::top()
{
  return this->function();
}

Node* Parser::parse()
{
  auto node = new Node(ND_Scope);

  while (this->check()) {
    auto& item = node->list.emplace_back(this->top());

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

    Error(ERR_UnexpectedToken, this->cur->prev)
        .suggest(this->cur->prev,
                 "expected semicolon after this token")
        .emit()
        .exit();
  }

  return node;
}
