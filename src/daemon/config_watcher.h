#pragma once

// clang-format off
#include <atomic>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include "config_watcher_if.h"
#include "src/inotify/inotify_if.h"
#include "src/io/io_if.h"
#include "src/log/logger.h"
#include "src/signal/signal_if.h"
// clang-format on

namespace hebpf {
namespace daemon {

class ConfigWatcher : public ConfigWatcherIf, public log::Loggable<log::Id::daemon> {
public:
  explicit ConfigWatcher(std::unique_ptr<inotify::InotifyIf> inotify);

  void registerConfigChangeEvents(std::string_view config, ConfigChangeCallback callback) override;

  bool setIoContext(std::weak_ptr<io::IoIf> io_ctx);

private:
  void loadConfig(std::string_view path);

  std::string config_path_;
  ConfigChangeCallback callback_;
  Configs last_config_;
  std::mutex config_mutex_;
  std::unique_ptr<inotify::InotifyIf> inotify_;
  std::shared_ptr<void> inotify_watcher_;
  std::weak_ptr<io::IoIf> io_ctx_;
};

} // namespace daemon
} // namespace hebpf
