#pragma once

// clang-format off
#include <functional>
#include <string>
#include <string_view>
// clang-format on

namespace hebpf {
namespace thread {

class ThreadIf {
public:
  virtual ~ThreadIf() = default;

  virtual void setName(std::string_view thread_name) = 0;
  virtual std::string getName() const = 0;
  virtual void start(std::function<void()> func) = 0;
  virtual void join() = 0;
  virtual bool joinable() const = 0;
};

} // namespace thread
} // namespace hebpf
