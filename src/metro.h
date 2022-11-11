#pragma once

#include <concepts>
#include <codecvt>
#include <cstring>
#include <functional>
#include <iostream>
#include <sstream>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

struct BuiltinFunc {
  using FuncType =
      std::function<Object*(Node*, std::vector<Object*> const&)>;

  char const* name;
  FuncType func;

  BuiltinFunc(char const* name, FuncType func);

  static std::vector<BuiltinFunc> const builtin_functions;
};

class Lexer {
 public:
  explicit Lexer(Source& source);

  Token* lex();

 private:
  void initialize();

  bool check();
  char peek();
  int match(std::string_view s);
  void pass_space();
  size_t pass_while(std::function<bool(char)> cond);

  char const* get_raw_ptr();

  Source& source;
  size_t position;
  size_t const length;

  std::vector<std::pair<size_t, size_t>> line_list;
};

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

  Node* expect_scope();

  bool eat_semi();
  void expect_semi();

  Node* new_value_nd(Object*);
  Node* new_assign(NodeKind kind, Token* token, Node* lhs, Node* rhs);

  Token* cur;
  Token* ate;
};

class MegaGC {
 public:
  MegaGC(MegaGC&&) = delete;
  MegaGC(MegaGC const&) = delete;

  static void execute();
  static void stop();

  static void append(Object* obj);

 private:
  MegaGC();
};

class Driver {
 public:
  Driver();

  Object* execute_script();

  int main(int argc, char** argv);

  static Source const& get_current_source();

 private:
  Source source;
  std::vector<std::wstring> argv;
};

enum ErrorKind {
  ERR_InvalidToken,

  ERR_InvalidSyntax,
  ERR_UnexpectedToken,
  ERR_ExpectedIdentifier,

  ERR_TypeMismatch,

  ERR_UndefinedVariable,
  ERR_UndefinedFunction,

  ERR_UninitializedVariable,

  ERR_BracketNotClosed,

  ERR_HereIsNotInsideOfFunc,

  ERR_InvalidOperator,
  ERR_MultiplyStringByNegative,

  ERR_IllegalFunctionCall,
  ERR_TooFewArguments,
  ERR_TooManyArguments,

  ERR_SubscriptOutOfRange,
};

class Error {
  enum LocationType : uint8_t {
    LOC_Position,
    LOC_Token,
    LOC_Node
  };

  struct ErrLocation {
    LocationType type;

    size_t begin;
    size_t end;
    size_t linenum;

    mutable size_t line_begin{};

    union {
      size_t pos;
      Token* token;
      Node* node;
    };

    ErrLocation(size_t);
    ErrLocation(Token*);
    ErrLocation(Node*);

    std::vector<std::string> trim_source() const;

    std::string to_string() const
    {
      return Utils::format(
          "{ErrLocation %p: type=%d, begin=%zu, end=%zu}", this,
          static_cast<int>(this->type), this->begin, this->end);
    }

    bool equals(ErrLocation const& loc) const
    {
      return this->type == loc.type && this->begin == loc.begin &&
             this->end == loc.end;
    }

   private:
    ErrLocation()
    {
    }
  };

  struct Suggestion {
    ErrLocation loc;
    std::string msg;

    bool _emitted{};

    Suggestion(ErrLocation loc, std::string&& msg)
        : loc(loc),
          msg(std::move(msg))
    {
    }
  };

  enum ErrTextFormat {
    EF_Main,
    EF_Help,
  };

 public:
  Error(ErrorKind kind, ErrLocation loc);

  Error& suggest(ErrLocation loc, std::string&& msg);

  Error& set_warn();

  Error& emit();

  static void check();

  [[noreturn]] void exit(int code = 1);

 private:
  std::string create_showing_text(ErrLocation const& loc,
                                  std::string const& msg,
                                  ErrTextFormat format,
                                  bool mix_suggest = true);

  std::vector<Suggestion*>* _find_suggest(ErrLocation const& loc)
  {
    for (auto&& [l, sv] : this->suggest_map)
      if (l.equals(loc)) return &sv;

    return nullptr;
  }

  ErrorKind kind;
  bool is_warn;

  ErrLocation loc;
  std::vector<Suggestion> suggests;

  std::vector<std::pair<ErrLocation, std::vector<Suggestion*>>>
      suggest_map;
};
