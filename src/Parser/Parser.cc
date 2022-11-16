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

    Error(ERR_UnexpectedToken, this->cur->prev)
        .suggest(this->cur->prev,
                 "expected semicolon after this token")
        .emit()
        .exit();
  }

  return node;
}
