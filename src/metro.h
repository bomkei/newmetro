#pragma once

#include <codecvt>
#include <functional>
#include <iostream>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace Utils {

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
  TYPE_List,
};

struct Type {
  TypeKind kind;
  size_t arr_depth;
  bool is_mutable;
  bool is_reference;

  std::vector<Type> elements;

  Type(TypeKind kind = TYPE_None)
      : kind(kind),
        arr_depth(0),
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

 protected:
  Object(Type const& type = TYPE_None)
      : type(type),
        ref_count(0)
  {
  }
};

template <class T, TypeKind kind, T val>
struct ObjImmediate : Object {
  T value;

  ObjImmediate()
      : Object(kind),
        value(val)
  {
  }

  std::string to_string() const override
  {
    return std::to_string(value);
  }
};

using ObjLong = ObjImmediate<int64_t, TYPE_Int, 0>;
// using ObjFloat = ObjImmediate<float, TYPE_Float, 0>;
using ObjBool = ObjImmediate<bool, TYPE_Bool, false>;
using ObjChar = ObjImmediate<wchar_t, TYPE_Char, 0>;
// using ObjString = ObjImmediate<std::wstring, TYPE_String, L"">;

struct ObjString : Object {
  std::wstring value;

  ObjString(std::wstring&& val = L"")
      : value(val)
  {
  }
};

struct ObjFloat : Object {
  float value;

  ObjFloat(float val = 0)
      : Object(TYPE_Float),
        value(val)
  {
  }
};

struct ObjTuple : Object {
  std::vector<Object*> elements;

  ObjTuple()
      : Object(TYPE_Tuple)
  {
  }
};

struct ObjList : Object {
  std::vector<Object*> list;

  ObjList()
      : Object(TYPE_List)
  {
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
  Token* prev;
  Token* next;

  std::string_view str;
  size_t pos;
  size_t endpos;

  Token(TokenKind kind)
      : kind(kind),
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

  ND_Value,
  ND_Variable,

  ND_Add,
  ND_Sub,
  ND_Mul,
  ND_Div,

  ND_Function,

  ND_Struct,
};

struct Node {
  NodeKind kind;
  Token* token;

  union {
    Node* nd[4];
  };
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

enum ErrorKind {
  ERR_InvalidToken,
  ERR_InvalidSyntax,
  ERR_TypeMismatch,
  ERR_UnexpectedToken,
};

class Error {
  enum LocationType {
    LOC_Position,
    LOC_Token,
    LOC_Node
  };

 public:
  Error(ErrorKind kind, size_t pos);
  Error(ErrorKind kind, Token* token);

  Error& suggest(Token* token, std::string const& msg);

  Error& emit();
  Error& emit_warn();

  void exit();

 private:
  ErrorKind kind;
  LocationType type;

  union {
    size_t pos;
    Token* token;
    Node* node;
  };
};