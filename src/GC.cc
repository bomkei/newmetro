#include <list>
#include <functional>
#include "types/Object.h"
#include "GC.h"
#include "Utils.h"

#define MTX_LOCK                \
  std::lock_guard<std::mutex> M \
  {                             \
    this->_mtx                  \
  }

static std::list<MetroGC*> _g_mgc_inst_list;

MetroGC::MetroGC()
    : _is_running(false)
{
  _g_mgc_inst_list.push_front(this);
}

MetroGC::~MetroGC()
{
  _g_mgc_inst_list.pop_front();
}

void MetroGC::execute()
{
  this->_is_running = true;

  this->_routine.reset(
      new std::thread(std::mem_fn(&MetroGC::_thread_routine), this));
}

void MetroGC::stop()
{
  this->_is_running = false;
  this->_routine->join();
}

Object*& MetroGC::append(Object* object)
{
  MTX_LOCK;

  for (auto&& p : this->_objects) {
    if (!p) {
      return p = object;
    }
  }

  return this->_objects.emplace_back(object);
}

void MetroGC::remove(Object* object)
{
  MTX_LOCK;

  for (auto&& p : this->_objects) {
    if (p == object) {
      p = nullptr;
      break;
    }
  }
}

void MetroGC::clean()
{
  MTX_LOCK;

  for (auto&& pObj : this->_objects) {
    if (pObj && pObj->ref_count == 0) {
      delete pObj;
      pObj = nullptr;
      break;
    }
  }
}

MetroGC* MetroGC::get_instance()
{
  return *_g_mgc_inst_list.begin();
}

void MetroGC::_thread_routine()
{
  while (this->_is_running) {
    this->clean();
    this->_sleep(1000);
  }
}

void MetroGC::_sleep(int milli)
{
  for (int i = 0; i < milli && this->_is_running; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
