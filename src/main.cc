#include "metro.h"

int main(int argc, char** argv)
{
  Source source{"test.txt"};

  Lexer lexer{source};

  auto token = lexer.lex();

  return 0;
}