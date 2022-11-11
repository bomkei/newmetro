#pragma once

#include "types/Source.h"

struct Object;

class Driver {
 public:
  Driver();

  Object* execute_script();

  int main(int argc, char** argv);

  static Source const& get_current_source();

 private:
  Source source;
  std::vector<std::wstring> argv;
};
