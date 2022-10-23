#include "metro.h"

static constexpr std::pair<ErrorKind, char const*> error_msg_list[]{
    {ERR_InvalidToken, "invalid token"},
    {ERR_InvalidSyntax, "invalid syntax"},
    {ERR_TypeMismatch, "type mismatch"},
    {ERR_UnexpectedToken, "unexpected token"},
};

static char const* get_err_msg(ErrorKind kind)
{
  for (auto&& [k, s] : error_msg_list) {
    if (k == kind) return s;
  }

  TODO_IMPL
}

Error::ErrLocation::ErrLocation(size_t pos)
    : begin(0),
      end(0),
      pos(pos)
{
}

Error::ErrLocation::ErrLocation(Token* token)
    : begin(0),
      end(0),
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

  std::cout << "error: " << msg << std::endl;

  return *this;
}

void Error::exit(int code) { std::exit(code); }