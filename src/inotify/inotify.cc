// clang-format off
#include "inotify.h"
#include <sys/inotify.h>
#include "src/common/assert.h"
#include "inotify_fd.h"
// clang-format on

namespace hebpf {
namespace inotify {

Inotify::Inotify(std::unique_ptr<FdIf> fd) : fd_(std::move(fd)) {
  if (fd_ == nullptr) {
    fd_ = std::make_unique<InotifyFd>();
  }
}

/**
 * @brief 增加一个监控项
 *
 * @param path 监控地址
 * @param mask 监控标识
 * @return true 添加成功
 * @return false 添加失败
 */
bool Inotify::addWatch(std::string_view path, uint32_t mask) {
  ASSERT(fd_ != nullptr);
  ASSERT(fd_->fd() != FD_INVALID);
  std::string path_str{path};
  int wd = inotify_add_watch(fd_->fd(), path_str.c_str(), mask);
  if (wd == -1) {
    LOG(warn, "Failed to add watch for {}: {}", path, strerror(errno));
    return false;
  }
  path_to_wd_[path_str] = wd;
  wd_to_path_[wd] = path_str;
  LOG(trace, "Added watch: {} (wd={})", path, wd);
  return true;
}

/**
 * @brief 移除一个监控项
 *
 * @param path 监控地址
 * @return true 移除成功
 * @return false 移除失败
 */
bool Inotify::removeWatch(std::string_view path) {
  ASSERT(fd_ != nullptr);
  ASSERT(fd_->fd() != FD_INVALID);
  auto it = path_to_wd_.find(std::string{path});
  if (it == path_to_wd_.end()) {
    LOG(warn, "Path {} not monitored, cannot be removed", path);
    return false;
  }
  int wd = it->second;
  if (inotify_rm_watch(fd_->fd(), wd) == -1) {
    LOG(warn, "Failed to remove watch for {}: {}", path, strerror(errno));
    return false;
  }
  path_to_wd_.erase(it);
  wd_to_path_.erase(wd);
  LOG(trace, "Removed watch: {} (wd={})", path, wd);
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
  ASSERT(cb_ != nullptr);

  const char *ptr = buffer;
  const char *end = ptr + length;
  const struct inotify_event *event{};
  while (ptr < end) {
    event = reinterpret_cast<const struct inotify_event *>(ptr);
    // 查找对应的监控路径
    auto it = wd_to_path_.find(event->wd);
    if (it == wd_to_path_.end()) {
      LOG(warn, "Received event for unknown wd: {}", event->wd);
    } else {
      if (event->len > 0) {
        cb_(event->name, event->mask);
      }
    }
    ptr += sizeof(struct inotify_event) + event->len;
  }
}

/**
 * @brief 列举所有监控项
 *
 * @return std::vector<std::string> 监控地址数组
 */
std::vector<std::string> Inotify::listWatches() const {
  std::vector<std::string> result{};
  for (const auto &[path, wd] : path_to_wd_) {
    result.emplace_back(path);
  }
  return result;
}

int Inotify::getFd() const noexcept {
  ASSERT(fd_ != nullptr);
  ASSERT(fd_->fd() != FD_INVALID);
  return fd_->fd();
}

} // namespace inotify
} // namespace hebpf
