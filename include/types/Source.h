#pragma once

#include <string>
#include <vector>

struct Source {
  std::string path;
  std::string text;

  std::vector<std::pair<size_t, size_t>> line_range_list;

  Source();
  Source(char const* path);

  bool readfile(char const* path);

  void init_line_list();

  // linenum, begin, end
  std::tuple<size_t, size_t, size_t> get_line(size_t pos) const;
};
