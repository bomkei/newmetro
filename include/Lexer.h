#pragma once

#include <string>
#include <vector>
#include <functional>

struct Token;
struct Source;

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
