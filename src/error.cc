#include "metro.h"

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
    case ND_Type:
    case ND_Argument:
    case ND_True:
    case ND_False:
    case ND_Value:
    case ND_Variable:
      break;

    case ND_List:
    case ND_Tuple:

    default:
      alertfmt("%d", node->kind);
      TODO_IMPL
  }

  return {node->token, node->token};
}

Error::ErrLocation::ErrLocation(size_t pos)
    : begin(0),
      end(0),
      pos(pos)
{
}

Error::ErrLocation::ErrLocation(Token* token)
    : begin(token->pos),
      end(token->endpos),
      token(token)
{
}

Error::ErrLocation::ErrLocation(Node* node)
    : begin(0),
      end(0),
      node(node)
{
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

  std::cout << "error: " << this->loc.token->str << " " << msg
            << std::endl;

  return *this;
}

void Error::exit(int code) { std::exit(code); }