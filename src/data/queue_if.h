#pragma once

// clang-format off
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {

constexpr int MAX_QUEUE_SIZE = 4 * 1024 * 1024;

template <typename T>
class QueueIf {
public:
  virtual ~QueueIf() = default;

  virtual void push(const T &elem) = 0;
  virtual void push(T &&elem) = 0;
  virtual std::optional<T> pop() = 0;
  virtual std::optional<T> tryPop() = 0;
  virtual bool empty() const = 0;
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool full() const = 0;
};

using QueueDaemonMonitor = QueueIf<nlohmann::json>;

} // namespace hebpf
