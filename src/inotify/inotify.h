#pragma once

// clang-format off
#include <memory>
#include <unordered_map>
#include <vector>
#include "src/log/logger.h"
#include "inotify_if.h"
#include "src/fd/fd_if.h"
// clang-format on

namespace hebpf {
namespace inotify {

class Inotify : public InotifyIf, public log::Loggable<log::Id::inotify> {
public:
  explicit Inotify(std::unique_ptr<FdIf> fd = nullptr);

  bool addWatch(std::string_view path, uint32_t mask) override;
  bool removeWatch(std::string_view path) override;
  void setInotifyCb(InotifyCb cb) override;
  void processInotify(const char *buffer, size_t length) override;
  int getFd() const noexcept override;

  std::vector<std::string> listWatches() const;

private:
  std::unique_ptr<FdIf> fd_;
  std::unordered_map<std::string, int> path_to_wd_;
  std::unordered_map<int, std::string> wd_to_path_;
  InotifyCb cb_;
};

} // namespace inotify
} // namespace hebpf
