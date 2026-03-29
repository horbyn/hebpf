#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "config_watcher_if.h"
#include "src/inotify/inotify_manager_if.h"
#include "src/io/io_if.h"
#include "src/log/logger.h"
#include "src/signal/signal_if.h"
#include "src/subscribe/publisher_if.h"
// clang-format on

namespace hebpf {
namespace daemon {

class ConfigWatcher : public ConfigWatcherIf,
                      public subscribe::PublisherIf,
                      public log::Loggable<log::Id::daemon> {
public:
  explicit ConfigWatcher(std::weak_ptr<inotify::InotifyManagerIf> inotify_manager);
  ~ConfigWatcher();

  void attach(std::shared_ptr<subscribe::SubscriberIf> subscriber) override;
  void detach(std::shared_ptr<subscribe::SubscriberIf> subscriber) override;
  void notify() override;

  void startWatching(std::string_view config);

private:
  void loadConfig();
  void clear();

  std::string config_path_;
  Configs last_config_;
  std::mutex config_mutex_;

  std::vector<std::weak_ptr<subscribe::SubscriberIf>> subscribers_;
  std::mutex subscribers_mutex_;

  std::weak_ptr<inotify::InotifyManagerIf> inotify_mngr_;
};

} // namespace daemon
} // namespace hebpf
