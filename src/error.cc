#include "metro.h"

#define COL_ERROR "\033[37;1;4m"

static constexpr std::pair<ErrorKind, char const*> error_msg_list[]{
    {ERR_InvalidToken, "invalid token"},
    {ERR_InvalidSyntax, "invalid syntax"},
    {ERR_UnexpectedToken, "unexpected token"},
    {ERR_ExpectedIdentifier, "expected identifier"},
    {ERR_TypeMismatch, "type mismatch"},
    {ERR_UndefinedVariable, "undefined variable name"},
    {ERR_UndefinedFunction, "undefined function name"},
    {ERR_UninitializedVariable, "variable is not uninitialized"},
    {ERR_BracketNotClosed, "bracket not closed"},
    {ERR_HereIsNotInsideOfFunc, "here is not inside of function"},
    {ERR_InvalidOperator, "invalid operator"},
    {ERR_MultiplyStringByNegative,
     "multiply string by negative value"},
    {ERR_IllegalFunctionCall, "illegal function call"},
    {ERR_TooFewArguments, "too few arguments"},
    {ERR_TooManyArguments, "too many arguments"},
};

static char const* get_err_msg(ErrorKind kind)
{
  for (auto&& [k, s] : error_msg_list) {
    if (k == kind) return s;
  }

  TODO_IMPL
}

static std::pair<Token*, Token*> get_token_range(Node* node)
{
  switch (node->kind) {
    case ND_None:
    case ND_SelfFunc:
    case ND_Type:
    case ND_Argument:
    case ND_True:
    case ND_False:
    case ND_Value:
    case ND_Variable:
      break;

    case ND_Callfunc: {
      auto [x, y] = get_token_range(node->nd_callfunc_functor);

      if (node->list.empty()) {
        return {x, y->next->next};
      }

      auto last = get_token_range(*node->list.rbegin());

      if (auto first = get_token_range(node->list[0]);
          first.second->endpos < x->pos) {
        if (node->list.size() == 1) {
          return {first.first, y->next->next};
        }

        return {first.first,
                get_token_range(*node->list.rbegin()).second->next};
      }

      return {x, last.second->next};
    }

    case ND_List:
    case ND_Tuple: {
      if (node->list.empty()) {
        return {node->token, node->token->next};
      }

      return {node->token,
              get_token_range(*node->list.rbegin()).second};
    }

    case ND_If:
    case ND_For:
    case ND_While:
    case ND_Let:
    case ND_Scope:
    case ND_Function:
    case ND_Struct:
      TODO_IMPL;

    default:
      return {get_token_range(node->nd_lhs).first,
              get_token_range(node->nd_rhs).second};
  }

  return {node->token, node->token};
}

Error::ErrLocation::ErrLocation(size_t pos)
    : begin(0),
      end(0),
      linenum(1),
      pos(pos)
{
  auto const& source = Driver::get_current_source();

  for (size_t i = 0; i < pos; i++) {
    if (source.text[i] == '\n') {
      this->linenum++;
    }
  }
}

Error::ErrLocation::ErrLocation(Token* token)
    : begin(token->pos),
      end(token->endpos),
      linenum(token->linenum),
      token(token)
{
}

Error::ErrLocation::ErrLocation(Node* node)
    : begin(0),
      end(0),
      linenum(0),
      node(node)
{
  auto [x, y] = get_token_range(node);

  if (y->pos >= x->pos) {
    this->linenum = x->linenum;
    this->begin = x->pos;
    this->end = y->endpos;
  }
  else {
    this->linenum = y->linenum;
    this->begin = y->pos;
    this->end = x->endpos;
  }
}

std::vector<std::string> Error::ErrLocation::trim_source()
{
  auto tbegin = this->begin;
  auto tend = this->end;

  auto const& source = Driver::get_current_source();

  while (tbegin > 0 && source.text[tbegin - 1] != '\n') {
    tbegin--;
  }

  while (tend < source.text.length() && source.text[tend] != '\n') {
    tend++;
  }

  auto ret = source.text.substr(tbegin, tend - tbegin);

  ret.insert(this->end - tbegin, COL_DEFAULT);
  ret.insert(this->begin - tbegin, COL_ERROR);

  std::vector<std::string> vec;
  std::string line;

  for (size_t i = tbegin; auto&& c : ret) {
    if (c == '\n') {
      vec.emplace_back(line + COL_DEFAULT);

      if (this->begin <= i && i < this->end) {
        line = COL_ERROR;
      }
      else {
        line.clear();
      }

      continue;
    }

    line.push_back(c);
    i++;
  }

  vec.emplace_back(line);

  return vec;
}

Error::Error(ErrorKind kind, Error::ErrLocation loc)
    : kind(kind),
      loc(loc),
      is_warn(false)
{
}

Error& Error::suggest(Error::ErrLocation loc, std::string const& msg)
{
  std::cout << msg << std::endl;

  return *this;
}

Error& Error::set_warn()
{
  this->is_warn = true;
  return *this;
}

Error& Error::emit()
{
  auto msg = get_err_msg(this->kind);

  auto const& source = Driver::get_current_source();
  auto const trimmed = this->loc.trim_source();

  if (this->is_warn)
    std::cout << COL_YELLOW << "warning: " << msg << std::endl;
  else
    std::cout << COL_RED << "error: " << msg << std::endl;

  std::cout << COL_CYAN << " --> " << source.path << ":"
            << this->loc.linenum << std::endl
            << COL_DEFAULT << "       |" << std::endl;

  for (auto linenum = this->loc.linenum; auto&& line : trimmed) {
    std::cout << Utils::format("%6d | ", linenum) << line
              << std::endl;

    linenum++;
  }

  std::cout << "       |\n\n";

  return *this;
}

void Error::exit(int code) { std::exit(code); }