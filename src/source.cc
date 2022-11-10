#include <fstream>
#include "metro.h"

Source::Source() {}

Source::Source(char const* path) { this->readfile(path); }

bool Source::readfile(char const* path)
{
  std::ifstream ifs{path};
  std::string line;

  if (ifs.fail()) {
    return false;
  }

  while (std::getline(ifs, line)) {
    this->text += line + '\n';
  }

  this->path = path;

  this->init_line_list();

  return true;
}

void Source::init_line_list()
{
  size_t a{};

  for (size_t b = 0; b < this->text.length(); b++) {
    if (this->text[b] == '\n') {
      this->line_range_list.emplace_back(a, b);
      a = b + 1;
    }
  }
}

std::tuple<size_t, size_t, size_t> Source::get_line(size_t pos) const
{
  for (size_t i = 1; auto&& [a, b] : this->line_range_list) {
    if (a <= pos && pos <= b) {
      return {i, a, b};
    }

    i++;
  }

  return {};
}
