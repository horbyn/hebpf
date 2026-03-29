#pragma once

// clang-format off
#include <memory>
#include "src/log/logger.h"
#include "inotify_if.h"
#include "src/fd/fd_if.h"
// clang-format on

namespace hebpf {
namespace inotify {

class Inotify : public InotifyIf, public log::Loggable<log::Id::inotify> {
public:
  explicit Inotify(std::unique_ptr<FdIf> fd = {});

  int addWatch(std::string_view path, uint32_t mask) override;
  bool removeWatch(int watch_fd) override;
  void setInotifyCb(InotifyCb cb) override;
  void processInotify(const char *buffer, size_t length) override;
  int getFd() const noexcept override;

private:
  std::unique_ptr<FdIf> fd_;
  InotifyCb cb_;
};

} // namespace inotify
} // namespace hebpf
