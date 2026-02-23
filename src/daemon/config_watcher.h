#pragma once

// clang-format off
#include <atomic>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include "src/daemon/configs.hpp"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace daemon {

using ConfigChangeCallback = std::function<void(const Configs &)>;

class ConfigWatcher : public log::Loggable<log::Id::daemon> {
public:
  explicit ConfigWatcher(std::string_view config_path);
  ~ConfigWatcher();

  void start(ConfigChangeCallback callback);
  void stop();

private:
  void watchLoop() noexcept;

  std::string config_path_;
  std::atomic<bool> running_{false};
  std::thread watch_thread_;
  ConfigChangeCallback callback_;
  Configs last_config_;
  std::mutex config_mutex_;
};

} // namespace daemon
} // namespace hebpf
