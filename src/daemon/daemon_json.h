#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"
#include "loader_json.h"
// clang-format on

namespace hebpf {
namespace daemon {

class DaemonJson final {
public:
  explicit DaemonJson() = default;
  explicit DaemonJson(bool running, bool queue_empty, bool queue_full, const LoaderJson &loader);

  void setRunning(bool running);
  bool getRunning() const;

  void setQueueEmpty(bool queue_empty);
  bool getQueueEmpty() const;

  void setQueueFull(bool queue_full);
  bool getQueueFull() const;

  void setLoader(const LoaderJson &loader);
  LoaderJson getLoader() const;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(DaemonJson, running, queue_empty, queue_full, loader);

private:
  bool running;
  bool queue_empty;
  bool queue_full;
  LoaderJson loader;
};

} // namespace daemon
} // namespace hebpf
