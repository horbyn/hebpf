#pragma once

// clang-format off
#include <thread>
#include "thread_if.h"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace thread {

class Thread : public ThreadIf, public log::Loggable<log::Id::common> {
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
