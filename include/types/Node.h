#pragma once

#include "types/Token.h"
#include "types/Object.h"

#define nd_lhs uni_nd[0]
#define nd_rhs uni_nd[1]

#define nd_arg_name uni_token
#define nd_arg_type uni_nd[1]

#define nd_value uni_object
#define nd_variable_name uni_token

#define nd_callfunc_functor uni_nd[0]

#define nd_func_return_type uni_nd[0]
#define nd_func_code uni_nd[1]

#define nd_if_cond uni_nd[0]
#define nd_if_true uni_nd[1]
#define nd_if_false uni_nd[2]

#define nd_for_iterator uni_nd[0]
#define nd_for_range uni_nd[1]
#define nd_for_loop_code uni_nd[2]

#define nd_return_expr uni_nd[0]

#define nd_let_name uni_token
#define nd_let_type uni_nd[1]
#define nd_let_init uni_nd[2]

enum NodeKind {
  ND_None,
  ND_SelfFunc,

  ND_Type,
  ND_Argument,

  ND_True,
  ND_False,

  ND_Value,

  ND_List,
  ND_EmptyList,

  ND_Tuple,

  ND_Variable,
  ND_Callfunc,

  ND_Subscript,
  ND_MemberAccess,

  ND_Add,
  ND_Sub,
  ND_Mul,
  ND_Div,
  ND_Mod,

  ND_LShift,
  ND_RShift,

  ND_Bigger,         // >
  ND_BiggerOrEqual,  // >=
  ND_Equal,
  ND_NotEqual,

  ND_BitAnd,
  ND_BitXor,
  ND_BitOr,

  ND_Range,

  ND_LogAnd,
  ND_LogOr,

  ND_Assign,

  ND_If,
  ND_For,
  // ND_Loop,
  ND_While,
  ND_Return,

  ND_Let,

  ND_Scope,

  ND_Function,

  ND_Struct,
};

struct Node {
  NodeKind kind;
  Token* token;

  union {
    Node* uni_nd[4]{0};

    struct {
      Token* uni_token;
      Object* uni_object;
      bool uni_bval[4];
    };
  };

  std::vector<Node*> list;

  Node(NodeKind kind, Token* token = nullptr);
  Node(NodeKind kind, Token* token, Node* lhs, Node* rhs);

  // ノード追加
  Node*& append(Node* node)
  {
    return this->list.emplace_back(node);
  }

  static Node* new_list(NodeKind kind, Token* token, Node* first)
  {
    auto x = new Node(kind, token);

    x->list.emplace_back(first);

    return x;
  }
};
