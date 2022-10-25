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

  return true;
}