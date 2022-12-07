#pragma once

#include <map>
#include <list>
#include <limits>

#include "types/Token.h"
#include "types/Object.h"

class MetroGC;
class Evaluator {
  struct Variable {
    Object* value;
    std::string_view name;

    Variable(Object* obj, std::string_view name)
        : value(obj),
          name(name)
    {
    }
  };

  struct Scope {
    Node* node;
    std::vector<Variable> variables;

    bool is_skipped;
    Object* lastval;

    size_t cur_index;

    explicit Scope(Node* node)
        : node(node),
          is_skipped(false),
          lastval(nullptr),
          cur_index(0)
    {
    }

    Variable* find_var(Token* name)
    {
      for (auto&& v : this->variables) {
        if (v.name == name->str) return &v;
      }

      return nullptr;
    }
  };

  struct CallStack {
    Node* func;
    bool is_returned;
    Object* result;

    CallStack(Node* func);
  };

  struct LoopContext {
    Node* node;
    Object* result;

    bool is_breaked;

    Scope& scope;

    LoopContext(Node* node, Scope& scope);
  };

 public:
  Evaluator(MetroGC&);
  ~Evaluator();

  Object* eval(Node* node);
  Object*& eval_lvalue(Node* node);

  Object* compute_expr(Node* node, Object* lhs, Object* rhs);
  Object*& compute_subscript(Node* node, Object* lhs, Object* index);
  Object*& compute_member_variable();

  Object* eval_scope(Scope& scope, Node* node);

 private:
  //
  // find the variable matching with name->str in all entered scopes
  Object*& get_var(Token* name);

  //
  // adjust the type of object for compute expr-node.
  void adjust_object_type(Object*& lhs, Object*& rhs);

  //
  // create a new scope from node(ND_Scope) and append it to stack
  Scope& enter_scope(Node* node);

  //
  // remove scope
  void leave_scope();

  //
  // get current scope
  Scope& get_cur_scope();

  CallStack& get_cur_call_stack();

  //
  // continue current loop
  LoopContext& enter_for_loop(Node* node);

  //
  // break current loop
  void leave_loop();

  //
  // get the LoopContext just current running
  LoopContext& get_cur_loop_context();

  //
  // loop condition controllers
  void loop_continue();
  void loop_break();

  //
  // check arguments
  void check_user_func_args(Node* node, Node* nd_func, Scope& scope);

  std::list<Scope> scope_stack;
  std::list<CallStack> call_stack;

  std::list<LoopContext> loop_stack;

  std::map<Node*, ObjFunction*> func_obj_map;

  MetroGC& _gc;
};
