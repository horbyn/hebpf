#pragma once

// clang-format off
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "src/fd/fd_if.h"
#include "src/io/io_if.h"
#include "src/log/logger.h"
#include "inotify_manager_if.h"
// clang-format on

namespace hebpf {
namespace inotify {

class InotifyManager : public InotifyManagerIf, public log::Loggable<log::Id::inotify> {
public:
  explicit InotifyManager(std::unique_ptr<InotifyIf> inotify = {});

  bool watchConfig(std::string_view path, InotifyCb on_change) override;
  bool goodbyeConfig(std::string_view path) override;

  void initManager(std::weak_ptr<io::IoIf> io_ctx);
  void destroyManager();

private:
  struct WatchInfo {
    int watch_fd{FD_INVALID};
    std::string watch_config{};
    InotifyCb on_change{};
  };
  using MapType = std::unordered_map<std::string, std::vector<WatchInfo>>;
  using MapIterator = MapType::iterator;
  using VecIterator = std::vector<WatchInfo>::iterator;

  std::pair<MapIterator, VecIterator> findWatchInfo(std::string_view parent_dir,
                                                    std::string_view path_str);
  static std::string getParentDir(std::string_view path);
  void onInotifyEvent(std::string_view name, uint32_t mask);

  std::mutex mutex_;
  std::unique_ptr<InotifyIf> inotify_;
  std::shared_ptr<void> watcher_;
  std::weak_ptr<io::IoIf> io_ctx_;

  MapType map_;
};

} // namespace inotify
} // namespace hebpf
