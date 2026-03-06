#pragma once

// clang-format off
#include <thread>
#include "thread_if.h"
// clang-format on

namespace hebpf {
namespace thread {

class Thread : public ThreadIf {
public:
  ~Thread();

  void setName(std::string_view thread_name);
  std::string getName() const;
  void start(std::function<void()> func);
  void join();
  bool joinable() const;

private:
  std::string name_;
  std::thread thread_;
};

} // namespace thread
} // namespace hebpf
