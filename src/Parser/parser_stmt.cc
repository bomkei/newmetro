#include "types/Object.h"
#include "types/Node.h"
#include "types/Token.h"
#include "types/BuiltinFunc.h"
#include "Error.h"
#include "Utils.h"
#include "Parser.h"

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
