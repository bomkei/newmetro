#include "Lexer.h"
#include "Parser.h"
#include "Evaluator.h"
#include "Driver.h"
#include "GC.h"

#include "Compiler/Compiler.h"

#include "Utils.h"

static Driver* __inst;

Driver::Driver()
{
  __inst = this;
}

Object* Driver::execute_script()
{
  MetroGC gc;

  Lexer lexer{this->source};

  auto token = lexer.lex();

  Parser parser{token};

  auto node = parser.parse();

  Compiler compiler;

  compiler.compile_node(node);

  auto const& codes = compiler.get_compiled_codes();

  for (auto&& op : codes) {
    alertios(op.to_string());
  }

  exit(1);

  return nullptr;

  // Evaluator eval{gc};

  // auto obj = eval.eval(node);

  // return obj;
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
