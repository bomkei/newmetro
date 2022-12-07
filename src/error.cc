#include <iostream>
#include <sstream>

#include "types/Token.h"
#include "types/Node.h"

#include "Utils.h"
#include "Driver.h"

#include "Error.h"

#define COL_ERROR "\033[37;1;4m"

#define COL_ERR_CYAN_UNDERLINE_BOLD_BRIGHT "\e[96;4;1m"

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
    {ERR_SubscriptOutOfRange, "subscript out of range"},
    {ERR_ValueOutOfRange, "value out of range"},
};

static size_t err_emitted_count{};

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
      auto first = get_token_range(node->nd_lhs).first;
      auto second = get_token_range(node->nd_rhs).second;
      return {first != nullptr ? first : node->token, second};
  }

  return {node->token, node->token};
}

Error::ErrLocation::ErrLocation(size_t pos)
    : type(LOC_Position),
      begin(0),
      end(0),
      pos(pos)
{
  auto const& source = Driver::get_current_source();

  this->linenum = std::get<0>(source.get_line(pos));
}

Error::ErrLocation::ErrLocation(Token* token)
    : type(LOC_Token),
      begin(token->pos),
      end(token->endpos),
      linenum(token->linenum),
      token(token)
{
}

Error::ErrLocation::ErrLocation(Node* node)
    : type(LOC_Node),
      begin(0),
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

std::vector<std::string> Error::ErrLocation::trim_source() const
{
  auto const& source = Driver::get_current_source();

  auto tbegin = std::get<1>(source.get_line(this->begin));
  auto tend = std::get<2>(source.get_line(this->end));

  this->line_begin = tbegin;

  auto ret = source.text.substr(tbegin, tend - tbegin);

  ret.insert(this->end - tbegin, COL_DEFAULT);
  ret.insert(this->begin - tbegin, COL_ERROR);

  std::vector<std::string> vec;
  std::string line;

  for (size_t i = tbegin; auto&& c : ret) {
    if (c == '\n') {
      vec.emplace_back(line);

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

std::string Error::ErrLocation::to_string() const
{
  return Utils::format(
      "{ErrLocation %p: type=%d, begin=%zu, end=%zu}", this,
      static_cast<int>(this->type), this->begin, this->end);
}

bool Error::ErrLocation::equals(ErrLocation const& loc) const
{
  return this->type == loc.type && this->begin == loc.begin &&
         this->end == loc.end;
}

Error::Suggestion::Suggestion(ErrLocation loc, std::string&& msg)
    : loc(loc),
      msg(std::move(msg))
{
}

Error::Error(ErrorKind kind, Error::ErrLocation loc)
    : kind(kind),
      loc(loc),
      is_warn(false)
{
}

Error& Error::suggest(Error::ErrLocation loc, std::string&& msg)
{
  auto& S = this->suggests.emplace_back(loc, std::move(msg));

  if (auto V = this->_find_suggest(loc); V) {
    V->emplace_back(&S);
  }
  else {
    this->suggest_map.emplace_back(loc, std::vector<Suggestion*>{&S});
  }

  return *this;
}

Error& Error::set_warn()
{
  this->is_warn = true;
  return *this;
}

//
// Create text for show in console
std::string Error::create_showing_text(ErrLocation const& loc,
                                       std::string const& msg,
                                       Error::ErrTextFormat format,
                                       bool mix_suggest)
{
  std::stringstream ss;

  auto const& source = Driver::get_current_source();
  auto trimmed = loc.trim_source();

  ss << msg << std::endl
     << COL_ERR_CYAN_UNDERLINE_BOLD_BRIGHT << " --> " << source.path
     << ":" << loc.linenum << ":" << loc.begin - loc.line_begin + 1
     << std::endl
     << COL_DEFAULT << "       |" << std::endl;

  for (auto linenum = loc.linenum; auto&& line : trimmed) {
    line = Utils::format(COL_DEFAULT "%6d | " COL_DEFAULT, linenum) +
           line;

    linenum++;
  }

  if (mix_suggest) {
    size_t const ix = trimmed.size();

    auto space = trimmed.emplace_back(
        "       |" + std::string(trimmed.rbegin()->length(), ' '));

    for (auto&& [L, SV] : this->suggest_map) {
      for (auto&& S : SV) {
        if (loc.begin <= L.begin && L.end <= loc.end) {
          auto linepos = L.end - std::get<1>(source.get_line(L.end));

          trimmed[ix][8 + linepos] = '~';

          trimmed.emplace_back("       | " +
                               std::string(linepos - 1, ' ') +
                               COL_MAGENTA + S->msg + COL_DEFAULT);

          S->_emitted = true;
        }
      }
    }
  }

  for (auto&& line : trimmed) {
    ss << line << std::endl;
  }

  ss << "       |\n\n";

  return ss.str();
}

std::vector<Error::Suggestion*>* Error::_find_suggest(
    ErrLocation const& loc)
{
  for (auto&& [l, sv] : this->suggest_map)
    if (l.equals(loc)) return &sv;

  return nullptr;
}

Error& Error::emit()
{
  auto const col = this->is_warn ? COL_YELLOW : COL_RED;

  auto msg = Utils::format(
      "%s%s" COL_ERR_CYAN_UNDERLINE_BOLD_BRIGHT "[E%04d]\e[0m%s: %s",
      col, this->is_warn ? "warning" : "error",
      static_cast<int>(this->kind), col, get_err_msg(this->kind));

  // show main message
  std::cout << this->create_showing_text(this->loc, msg, EF_Main);

  // show suggests
  for (auto&& S : this->suggests) {
    if (!S._emitted) {
      std::cout << this->create_showing_text(
          S.loc, COL_MAGENTA "help: " + S.msg, EF_Help);
    }
  }

  if (!this->is_warn) {
    err_emitted_count++;
  }

  return *this;
}

void Error::check()
{
  if (err_emitted_count >= 10) {
    std::exit(1);
  }
}

void Error::exit(int code)
{
  std::exit(code);
}