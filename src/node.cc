#include "metro.h"

Node::Node(NodeKind kind, Token* token)
    : kind(kind),
      token(token)
{
}

Node::Node(NodeKind kind, Token* token, Node* lhs, Node* rhs)
    : kind(kind),
      token(token)
{
  this->nd_lhs = lhs;
  this->nd_rhs = rhs;
}
