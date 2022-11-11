#pragma once

#include <string>
#include <vector>

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

struct Token;
struct Node;

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

    std::string to_string() const;

    bool equals(ErrLocation const& loc) const;

   private:
    ErrLocation()
    {
    }
  };

  struct Suggestion {
    ErrLocation loc;
    std::string msg;

    bool _emitted{};

    Suggestion(ErrLocation loc, std::string&& msg);
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

  std::vector<Suggestion*>* _find_suggest(ErrLocation const& loc);

  ErrorKind kind;
  bool is_warn;

  ErrLocation loc;
  std::vector<Suggestion> suggests;

  std::vector<std::pair<ErrLocation, std::vector<Suggestion*>>>
      suggest_map;
};
