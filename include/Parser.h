#pragma once

#include <string>
#include <functional>
#include "types/Node.h"

struct Token;
struct Object;

//
// 構文解析
class Parser {
 public:
  explicit Parser(Token* token);

  Node* atom();
  Node* factor();

  Node* statement();

  Node* callfunc();

  Node* member_access();
  Node* unary();

  Node* mul();
  Node* add();

  Node* shift();

  Node* compare();
  Node* equality();

  Node* bit_and();
  Node* bit_xor();
  Node* bit_or();

  Node* range();

  Node* log_and();
  Node* log_or();

  Node* assign();

  Node* expr();

  Node* function();
  Node* p_struct();
  Node* p_namespace();

  Node* top();

  Node* parse();

 private:
  bool check();
  void next();
  bool eat(std::string_view s);
  void expect(std::string_view s);

  //
  // 識別子を期待する
  // たべた場合は、ひとつ進めてからその識別子を返す
  // そうでなければエラー
  Token* expect_ident(bool allow_kwd = false);

  //
  // 型を期待
  Node* expect_type();

  Node* expect_scope(
      std::function<Node*(Parser*)> chi = &Parser::expr);

  bool eat_semi();
  void expect_semi();

  Node* new_value_nd(Object*);
  Node* new_assign(NodeKind kind, Token* token, Node* lhs, Node* rhs);

  Token* cur;
  Token* ate;
};
