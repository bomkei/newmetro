#include "metro.h"

int main(int argc, char** argv)
{
  Source source{"test.txt"};

  Lexer lexer{source};

  auto token = lexer.lex();

  Parser parser{token};

  auto node = parser.parse();

  return 0;
}