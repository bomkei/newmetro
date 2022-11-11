#pragma once

#include "types/Token.h"
#include "types/Object.h"

//
// 構文木を評価 ( 実行 )
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

    bool is_loop_continued{};

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
    Scope* scope;

    Object** pObj_iterator;
    Object** pObj_range;

    static LoopContext from_For(Node* nd_for);

   private:
    LoopContext();
  };

 public:
  Evaluator()
  {
  }

  Object* eval(Node* node);
  Object*& eval_lvalue(Node* node);

  Object* compute_expr(Node* node, Object* lhs, Object* rhs);
  Object*& compute_subscript(Node* node, Object* lhs, Object* index);
  Object*& compute_member_variable();

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

  std::list<Scope> scope_stack;
  std::list<Node*> call_stack;

  std::list<LoopContext> loop_stack;
};
