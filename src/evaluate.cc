#include "metro.h"

template <std::derived_from<Object> T = Object, class... Args>
T* gcnew(Args&&... args)
{
  auto obj = new T(std::forward<Args...>(args...));

  // append

  return obj;
}

Object* Evaluator::mt_add(Object* lhs, Object* rhs) {}

Object* Evaluator::eval(Node* node)
{
  if (!node) {
    return gcnew<ObjNone>();
  }

  switch (node->kind) {
  }

  auto lhs = this->eval(node->nd_lhs);
  auto rhs = this->eval(node->nd_rhs);

  switch (node->kind) {
    case ND_Add:
  };

  return
}