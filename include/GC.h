#pragma once;

struct Object;

class MetroGC {
 public:
  MetroGC(MetroGC&&) = delete;
  MetroGC(MetroGC const&) = delete;

  static void execute();
  static void stop();

  static void append(Object* obj);

 private:
  MetroGC();
};
