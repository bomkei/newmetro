#pragma once

#include <list>
#include <concepts>
#include <codecvt>
#include <cstring>
#include <functional>
#include <sstream>
#include <locale>
#include <string>
#include <vector>

#include "ColorDefine.h"

#define __FILE_EX__ __file_ex_fn__(__FILE__, "src")

// alert
#define alert                                                 \
  fprintf(stderr, COL_MAGENTA "\t#alert %s:%d\n" COL_DEFAULT, \
          __FILE_EX__, __LINE__)

// alertphase
#define alertphase(s)                                         \
  fprintf(stderr, "\033[35;1m" s COL_DEFAULT " from %s:%d\n", \
          __FILE_EX__, __LINE__)

// alertmsg
#define alertmsg(e...)                                         \
  fprintf(stderr,                                              \
          COL_YELLOW "\t#message: " COL_ALERTMSG #e COL_YELLOW \
                     " :from %s:%d\n" COL_DEFAULT,             \
          __FILE_EX__, __LINE__)

// alertfmt
#define alertfmt(fmt, e...)                                        \
  fprintf(stderr,                                                  \
          COL_YELLOW "\t#message: " fmt COL_ALERTMSG #e COL_YELLOW \
                     " :from %s:%d\n" COL_DEFAULT,                 \
          e, __FILE_EX__, __LINE__)

// alertios
#define alertios(e...)                                      \
  (std::cerr << COL_YELLOW "\t#message: " COL_ALERTMSG << e \
             << COL_YELLOW " :from " << __FILE_EX__ << ":"  \
             << __LINE__ << "\n" COL_DEFAULT)

// alertwarn
#define alertwarn(e...) alertmsg(COL_RED "#warning: " #e)

// alertctor
#define alertctor(_Name_)                                     \
  fprintf(stderr,                                             \
          COL_GREEN "\t#Constructing " COL_CYAN #_Name_       \
                    " (%p)" COL_GREEN ":%s:%d\n" COL_DEFAULT, \
          this, __FILE_EX__, __LINE__)

// alertctor
#define alertdtor(_Name_)                                            \
  fprintf(stderr,                                                    \
          COL_RED "\t#Destructing " COL_CYAN #_Name_ " (%p)" COL_RED \
                  ":%s:%d\n" COL_DEFAULT,                            \
          this, __FILE_EX__, __LINE__)

// alertwhere
#define alertwhere                                                \
  fprintf(stderr,                                                 \
          "\t" COL_MAGENTA "# here is in function " COL_YELLOW    \
          "'%s'" COL_MAGENTA " in " COL_GREEN "%s\n" COL_DEFAULT, \
          __func__, __FILE_EX__)

#define TODO_IMPL                                                \
  {                                                              \
    fprintf(stderr,                                              \
            COL_RED "\n\n# Not implemented error at " COL_YELLOW \
                    "%s:%d\n" COL_DEFAULT,                       \
            __FILE__, __LINE__);                                 \
    exit(1);                                                     \
  }

#define debug(e...) \
  {                 \
    e               \
  }

#define crash                                                     \
  {                                                               \
    alert;                                                        \
    fprintf(stderr, "\n#crashed at " __FILE__ ":%d\n", __LINE__); \
    exit(1);                                                      \
  }

inline char const* __file_ex_fn__(char const* a, char const* b)
{
  size_t const len = strlen(b);

  for (auto p = a;;)
    if (memcmp(++p, b, len) == 0) return p;

  return a;
}

namespace Utils {

using std::to_string;

template <class... Args>
std::string format(char const* fmt, Args&&... args)
{
  static char buf[0x1000];
  sprintf(buf, fmt, args...);
  return buf;
}

template <class callable_t>
using return_type_of_t = typename decltype(std::function{
    std::declval<callable_t>(0)}())::result_type;

template <class T>
concept convertible_to_string_with_method = requires(T const& x)
{
  {
    x.to_string()
    } -> std::convertible_to<std::string>;
};

template <convertible_to_string_with_method T>
inline auto to_string(T const& x) -> std::string
{
  return x.to_string();
}

template <class T>
concept convertible_to_string =
    convertible_to_string_with_method<T> ||
    std::is_convertible_v<T, std::string>;

template <convertible_to_string T>
std::string join(std::string const& s, std::vector<T> const& vec)
{
  std::string ret;

  for (auto last = &*vec.rbegin(); auto&& x : vec) {
    ret += to_string(x);
    if (last != &x) ret += s;
  }

  return ret;
}

template <class T, class F = std::function<std::string(T)>>
std::string join(std::string const& s, std::vector<T> const& vec,
                 F conv)
{
  std::string ret;

  for (auto last = &*vec.rbegin(); auto&& x : vec) {
    ret += conv(x);
    if (last != &x) ret += s;
  }

  return ret;
}

template <class T, class F = std::function<std::string(T)>>
std::string join(std::string const& s, std::list<T> const& list,
                 F conv)
{
  std::string ret;

  for (auto last = &*list.rbegin(); auto&& x : list) {
    ret += conv(x);
    if (last != &x) ret += s;
  }

  return ret;
}

class Converter {
  static inline std::wstring_convert<std::codecvt_utf8<wchar_t>,
                                     wchar_t>
      conv;

 public:
  // wide --> utf8
  static std::string to_utf8(std::wstring const& s)
  {
    return conv.to_bytes(s);
  }

  // utf8 --> wide
  static std::wstring to_wide(std::string const& s)
  {
    return conv.from_bytes(s);
  }
};

}  // namespace Utils
