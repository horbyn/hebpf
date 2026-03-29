// clang-format off
#include "inotify.h"
#include <sys/inotify.h>
#include "src/common/assert.h"
#include "inotify_fd.h"
// clang-format on

namespace hebpf {
namespace inotify {

Inotify::Inotify(std::unique_ptr<FdIf> fd) : fd_{std::move(fd)} {
  if (fd_ == nullptr) {
    fd_ = std::make_unique<InotifyFd>();
  }
}

/**
 * @brief 增加一个监控项
 *
 * @param path 监控地址
 * @param mask 监控标识
 * @return 监控项文件描述符，出错返回 FD_INVALID
 */
int Inotify::addWatch(std::string_view path, uint32_t mask) {
  ASSERT(fd_ != nullptr);
  ASSERT(fd_->fd() != FD_INVALID);
  std::string path_str{path};
  int wd = inotify_add_watch(fd_->fd(), path_str.c_str(), mask);
  if (wd == -1) {
    LOG(warn, "Failed to add watch for {}: {}", path, strerror(errno));
    return FD_INVALID;
  }
  LOG(trace, "Added watch: {} (wd={})", path, wd);
  return wd;
}

/**
 * @brief 移除一个监控项
 *
 * @param watch_fd 监控项文件描述符
 * @return true 移除成功
 * @return false 移除失败
 */
bool Inotify::removeWatch(int watch_fd) {
  ASSERT(fd_ != nullptr);
  ASSERT(fd_->fd() != FD_INVALID);
  if (inotify_rm_watch(fd_->fd(), watch_fd) == -1) {
    LOG(warn, "Failed to remove watch for {}: {}", watch_fd, strerror(errno));
    return false;
  }
  LOG(trace, "Removed watch: {}", watch_fd);
  return true;
}

/**
 * @brief 设置监控变更时要进行的回调
 *
 * @param cb 回调函数
 */
void Inotify::setInotifyCb(InotifyCb cb) { cb_ = std::move(cb); }

/**
 * @brief 解析 inotify 数据，解析过程中会调用设置给 inotify 的回调函数
 *
 * @param buffer 缓存
 * @param length 缓存大小
 */
void Inotify::processInotify(const char *buffer, size_t length) {
  ASSERT(cb_);

  const char *ptr = buffer;
  const char *end = ptr + length;
  const struct inotify_event *event{};
  while (ptr < end) {
    event = reinterpret_cast<const struct inotify_event *>(ptr);
    if (event->len > 0) {
      cb_(event->name, event->mask);
    }
    ptr += sizeof(struct inotify_event) + event->len;
  }
}

/**
 * @brief 获取 inotify 背后的 fd
 *
 * @return int 文件描述符
 */
int Inotify::getFd() const noexcept {
  ASSERT(fd_ != nullptr);
  return fd_->fd();
}

} // namespace inotify
} // namespace hebpf
