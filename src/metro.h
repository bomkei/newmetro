#pragma once

#include <concepts>
#include <codecvt>
#include <cstring>
#include <functional>
#include <iostream>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#define COL_BLACK "\e[30m"
#define COL_RED "\e[31m"
#define COL_GREEN "\033[32m"
#define COL_YELLOW "\033[33m"
#define COL_BLUE "\033[34m"
#define COL_MAGENTA "\033[35m"
#define COL_CYAN "\033[36;5m"
#define COL_WHITE "\033[37m"
#define COL_DEFAULT "\033[0m"

#define __FILE_EX__ __file_ex_fn__(__FILE__, "src")

// alert
#define alert                                                 \
  fprintf(stderr, COL_MAGENTA "\t#alert %s:%d\n" COL_DEFAULT, \
          __FILE_EX__, __LINE__)

// alertphase
#define alertphase(s)                                         \
  fprintf(stderr, "\033[35;1m" s COL_DEFAULT " from %s:%d\n", \
          __FILE_EX__, __LINE__)

// alertmsg
#define alertmsg(e...)                                         \
  fprintf(stderr,                                              \
          COL_YELLOW "\t#message: " COL_ALERTMSG #e COL_YELLOW \
                     " :from %s:%d\n" COL_DEFAULT,             \
          __FILE_EX__, __LINE__)

// alertfmt
#define alertfmt(fmt, e...)                                        \
  fprintf(stderr,                                                  \
          COL_YELLOW "\t#message: " fmt COL_ALERTMSG #e COL_YELLOW \
                     " :from %s:%d\n" COL_DEFAULT,                 \
          e, __FILE_EX__, __LINE__)

// alertios
#define alertios(e...)                                      \
  (std::cerr << COL_YELLOW "\t#message: " COL_ALERTMSG << e \
             << COL_YELLOW " :from " << __FILE_EX__ << ":"  \
             << __LINE__ << "\n" COL_DEFAULT)

// alertwarn
#define alertwarn(e...) alertmsg(COL_RED "#warning: " #e)

// alertctor
#define alertctor(_Name_)                                     \
  fprintf(stderr,                                             \
          COL_GREEN "\t#Constructing " COL_CYAN #_Name_       \
                    " (%p)" COL_GREEN ":%s:%d\n" COL_DEFAULT, \
          this, __FILE_EX__, __LINE__)

// alertctor
#define alertdtor(_Name_)                                            \
  fprintf(stderr,                                                    \
          COL_RED "\t#Destructing " COL_CYAN #_Name_ " (%p)" COL_RED \
                  ":%s:%d\n" COL_DEFAULT,                            \
          this, __FILE_EX__, __LINE__)

// alertwhere
#define alertwhere                                                \
  fprintf(stderr,                                                 \
          "\t" COL_MAGENTA "# here is in function " COL_YELLOW    \
          "'%s'" COL_MAGENTA " in " COL_GREEN "%s\n" COL_DEFAULT, \
          __func__, __FILE_EX__)

#define TODO_IMPL                                                \
  {                                                              \
    fprintf(stderr,                                              \
            COL_RED "\n\n# Not implemented error at " COL_YELLOW \
                    "%s:%d\n" COL_DEFAULT,                       \
            __FILE__, __LINE__);                                 \
    exit(1);                                                     \
  }

#define debug(e...) \
  {                 \
    e               \
  }

#define crash                                                     \
  {                                                               \
    alert;                                                        \
    fprintf(stderr, "\n#crashed at " __FILE__ ":%d\n", __LINE__); \
    exit(1);                                                      \
  }

inline char const* __file_ex_fn__(char const* a, char const* b)
{
  size_t const len = strlen(b);

  for (auto p = a;;)
    if (memcmp(++p, b, len) == 0) return p;

  return a;
}

namespace Utils {

using std::to_string;

template <class... Args>
std::string format(char const* fmt, Args&&... args)
{
  static char buf[0x1000];
  sprintf(buf, fmt, std::forward<Args...>(args...));
  return buf;
}

template <class callable_t>
using return_type_of_t = typename decltype(std::function{
    std::declval<callable_t>(0)}())::result_type;

template <class T>
concept convertible_to_string_with_method = requires(T const& x)
{
  {
    x.to_string()
    } -> std::convertible_to<std::string>;
};

template <convertible_to_string_with_method T>
inline auto to_string(T const& x) -> std::string
{
  return x.to_string();
}

template <class T>
concept convertible_to_string =
    convertible_to_string_with_method<T> ||
    std::is_convertible_v<T, std::string>;

template <convertible_to_string T>
std::string join(std::string const& s, std::vector<T> const& vec)
{
  std::string ret;

  for (auto last = &*vec.rbegin(); auto&& x : vec) {
    ret += to_string(x);
    if (last != &x) ret += s;
  }

  return ret;
}

template <class T, class F = std::function<std::string(T)>>
std::string join(std::string const& s, std::vector<T> const& vec,
                 F conv)
{
  std::string ret;

  for (auto last = &*vec.rbegin(); auto&& x : vec) {
    ret += conv(x);
    if (last != &x) ret += s;
  }

  return ret;
}

template <class T, class F = std::function<std::string(T)>>
std::string join(std::string const& s, std::list<T> const& list,
                 F conv)
{
  std::string ret;

  for (auto last = &*list.rbegin(); auto&& x : list) {
    ret += conv(x);
    if (last != &x) ret += s;
  }

  return ret;
}

class Converter {
  static inline std::wstring_convert<std::codecvt_utf8<wchar_t>,
                                     wchar_t>
      conv;

 public:
  // wide --> utf8
  static std::string to_utf8(std::wstring const& s)
  {
    return conv.to_bytes(s);
  }

  // utf8 --> wide
  static std::wstring to_utf32(std::string const& s)
  {
    return conv.from_bytes(s);
  }
};

}  // namespace Utils

enum TypeKind {
  TYPE_None,
  TYPE_Int,
  TYPE_Float,
  TYPE_Bool,
  TYPE_Char,
  TYPE_String,
  TYPE_Tuple,
  TYPE_Vector,
};

struct Type {
  TypeKind kind;
  bool is_mutable;
  bool is_reference;

  std::vector<Type> elements;

  Type(TypeKind kind = TYPE_None)
      : kind(kind),
        is_mutable(false),
        is_reference(false)
  {
  }

  std::string to_string() const;

  bool equals(TypeKind kind) const;
  bool equals(Type const& type) const;
};

struct Object {
  Type type;
  size_t ref_count;

  virtual std::string to_string() const = 0;
  virtual Object* clone() const = 0;

 protected:
  Object(Type const& type)
      : type(type),
        ref_count(0)
  {
  }
};

struct ObjNone : Object {
  ObjNone()
      : Object(TYPE_None)
  {
  }

  std::string to_string() const override { return "none"; }

  ObjNone* clone() const override { return new ObjNone; }
};

template <class T, TypeKind kind>
struct ObjImmediate : Object {
  T value;

  ObjImmediate(T val)
      : Object(kind),
        value(val)
  {
  }

  std::string to_string() const override
  {
    return std::to_string(this->value);
  }

  ObjImmediate* clone() const override
  {
    return new ObjImmediate<T, kind>(this->value);
  }
};

using ObjLong = ObjImmediate<int64_t, TYPE_Int>;
using ObjBool = ObjImmediate<bool, TYPE_Bool>;
using ObjChar = ObjImmediate<wchar_t, TYPE_Char>;

struct ObjString : Object {
  std::wstring value;

  ObjString(std::wstring const& val = L"")
      : Object(TYPE_String),
        value(val)
  {
  }

  std::string to_string() const override
  {
    return Utils::Converter::to_utf8(this->value);
  }

  ObjString* clone() const override
  {
    return new ObjString(this->value);
  }
};

struct ObjFloat : Object {
  float value;

  ObjFloat(float val = 0)
      : Object(TYPE_Float),
        value(val)
  {
  }

  std::string to_string() const override
  {
    return std::to_string(this->value);
  }

  ObjFloat* clone() const override
  {
    return new ObjFloat(this->value);
  }
};

struct ObjTuple : Object {
  std::vector<Object*> elements;

  ObjTuple()
      : Object(TYPE_Tuple)
  {
  }

  std::string to_string() const override
  {
    return "(" +
           Utils::join<Object*>(
               ", ", this->elements,
               [](auto x) { return x->to_string(); }) +
           ")";
  }

  ObjTuple* clone() const override
  {
    auto x = new ObjTuple;

    for (auto&& elem : this->elements) {
      x->elements.emplace_back(elem->clone());
    }

    return x;
  }
};

struct ObjVector : Object {
  std::vector<Object*> list;

  ObjVector()
      : Object(TYPE_Vector)
  {
  }

  std::string to_string() const override
  {
    return "[" +
           Utils::join<Object*>(
               ", ", this->list,
               [](auto x) { return x->to_string(); }) +
           "]";
  }

  ObjVector* clone() const override
  {
    auto x = new ObjVector;

    for (auto&& item : this->list) {
      x->list.emplace_back(item->clone());
    }

    return x;
  }
};

enum TokenKind {
  TOK_Immediate,
  TOK_Ident,
  TOK_Keyword,
  TOK_Punctuator,
  TOK_End
};

struct Token {
  TokenKind kind;
  TypeKind imm_kind;
  Token* prev;
  Token* next;

  std::string_view str;
  size_t pos;
  size_t endpos;

  Token(TokenKind kind)
      : kind(kind),
        imm_kind(TYPE_None),
        prev(nullptr),
        next(nullptr),
        pos(0),
        endpos(0)
  {
  }

  Token(TokenKind kind, Token* prev, size_t pos)
      : Token(kind)
  {
    this->prev = prev;
    this->pos = pos;

    this->prev->next = this;
  }
};

enum NodeKind {
  ND_None,

  ND_Type,
  ND_Argument,
  ND_ArgumentList,

  ND_Value,
  ND_List,
  ND_Tuple,

  ND_Variable,
  ND_Callfunc,

  ND_Add,
  ND_Sub,
  ND_Mul,
  ND_Div,

  ND_If,
  ND_For,
  // ND_Loop,
  ND_While,

  ND_Let,

  ND_Scope,

  ND_Function,

  ND_Struct,
};

/*

value:
  Object* obj;

variable:
  Token* name;

callfunc:
  Node* functor;

let:
  Token* name;
  Node* type;
  Node* init;

function:
  Token* name;
  Node* args;
  Node* return_type;
  Node* code;

if:
  Node* cond;
  Node* if_true;
  Node* if_false;

*/

#define nd_lhs uni_nd[0]
#define nd_rhs uni_nd[1]

#define nd_value uni_object
#define nd_variable_name uni_token

#define nd_callfunc_functor uni_nd[0]

#define nd_func_name uni_token
#define nd_func_args uni_nd[1]
#define nd_func_return_type uni_nd[2]
#define nd_func_code uni_nd[3]

#define nd_if_cond uni_nd[0]
#define nd_if_true uni_nd[1]
#define nd_if_false uni_nd[2]

#define nd_let_name uni_token
#define nd_let_type uni_nd[1]
#define nd_let_init uni_nd[2]

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
};

struct Source {
  std::string text;

  Source();
  Source(char const* path);

  bool readfile(char const* path);
};

class Lexer {
 public:
  explicit Lexer(Source& source);

  Token* lex();

 private:
  bool check();
  char peek();
  int match(std::string_view s);
  void pass_space();
  size_t pass_while(std::function<bool(char)> cond);

  char const* get_raw_ptr();

  size_t position;
  size_t const length;
  Source& source;
};

//
// 構文解析
class Parser {
 public:
  explicit Parser(Token* token);

  Node* factor();
  Node* callfunc();
  Node* mul();
  Node* add();

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
  Token* expect_ident();

  //
  // 型を期待
  Node* expect_type();

  Token* cur;
  Token* ate;
};

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

    size_t cur_index;

    explicit Scope(Node* node)
        : node(node),
          is_skipped(false),
          lastval(nullptr),
          cur_index(0)
    {
    }
  };

 public:
  Evaluator() {}

  Object* eval(Node* node);
  Object*& eval_lvalue(Node* node);

  Object* mt_add(Object* lhs, Object* rhs);
  Object* mt_sub(Object* lhs, Object* rhs);

 private:
  Scope& enter_scope(Node* node);
  void leave_scope();

  Scope& get_cur_scope();

  Object*& get_var(Token* name);

  std::list<Scope> scope_stack;
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

enum ErrorKind {
  ERR_InvalidToken,

  ERR_InvalidSyntax,
  ERR_UnexpectedToken,
  ERR_ExpectedIdentifier,

  ERR_TypeMismatch,

  ERR_UndefinedVariable,

  ERR_UninitializedVariable
};

class Error {
  enum LocationType {
    LOC_Position,
    LOC_Token,
    LOC_Node
  };

  struct ErrLocation {
    LocationType type;

    size_t begin;
    size_t end;

    union {
      size_t pos;
      Token* token;
      Node* node;
    };

    ErrLocation(size_t);
    ErrLocation(Token*);
    ErrLocation(Node*);

   private:
    ErrLocation() {}
  };

 public:
  Error(ErrorKind kind, ErrLocation loc);

  Error& suggest(ErrLocation loc, std::string const& msg);

  Error& set_warn();

  Error& emit();

  [[noreturn]] void exit(int code = 1);

 private:
  ErrorKind kind;

  ErrLocation loc;

  bool is_warn;
};