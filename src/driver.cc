#include "Lexer.h"
#include "Parser.h"
#include "Evaluator.h"
#include "Driver.h"

static Driver* __inst;

Driver::Driver()
{
  __inst = this;
}

Object* Driver::execute_script()
{
  Lexer lexer{this->source};

  auto token = lexer.lex();

  Parser parser{token};

  auto node = parser.parse();

  Evaluator eval;

  auto obj = eval.eval(node);

  return obj;
}

int Driver::main(int argc, char** argv)
{
  this->source.readfile("test.txt");

  auto res = this->execute_script();

  return 0;
}

Source const& Driver::get_current_source()
{
  return __inst->source;
}
