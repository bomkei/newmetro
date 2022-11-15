#pragma once;

#include <vector>
#include <thread>
#include <mutex>

struct Object;

class MetroGC {
 public:
  MetroGC();
  ~MetroGC();

  void execute();
  void stop();

  Object*& append(Object*);
  void remove(Object*);

  void clean();

  static MetroGC* get_instance();

 private:
  void _thread_routine();
  void _sleep(int milli);

  bool _is_running;
  std::vector<Object*> _objects;

  std::unique_ptr<std::thread> _routine;
  std::mutex _mtx;
};
