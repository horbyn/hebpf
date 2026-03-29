// clang-format off
#include "inotify_manager.h"
#include <sys/inotify.h>
#include <filesystem>
#include "src/common/assert.h"
#include "src/common/common.h"
#include "src/common/exception.h"
#include "inotify.h"
// clang-format on

namespace hebpf {
namespace inotify {

InotifyManager::InotifyManager(std::unique_ptr<InotifyIf> inotify) : inotify_{std::move(inotify)} {
  if (inotify_ == nullptr) {
    inotify_ = std::make_unique<Inotify>();
  }
}

/**
 * @brief 监控配置路径
 *
 * @param path 配置路径
 * @param on_change 回调函数
 * @return true 监控成功
 * @return false 监控失败
 */
bool InotifyManager::watchConfig(std::string_view path, InotifyCb on_change) {
  std::string path_str{path};
  auto watch_path_is_parent = getParentDir(path);
  auto [map_iter, vec_iter] = findWatchInfo(watch_path_is_parent, path_str);
  if (map_iter != map_.end() && vec_iter != map_iter->second.end()) {
    LOG(warn, "Path already watched: {}", path);
    return true;
  }

  std::lock_guard<std::mutex> lock{mutex_};

  int watch_fd{FD_INVALID};
  if (map_iter == map_.end()) {
    // 只会为父目录创建 Inotify 监控
    watch_fd = inotify_->addWatch(watch_path_is_parent, IN_CLOSE_WRITE | IN_MOVED_TO);
    if (watch_fd == FD_INVALID) {
      return false;
    }
  }

  // 只有数组第一个元素的 watch_fd 才有效，其余元素都是 FD_INVALID
  WatchInfo watch_info{.watch_fd = watch_fd, .watch_config = path_str, .on_change = on_change};
  if (map_iter == map_.end()) {
    map_[watch_path_is_parent] = {watch_info};
  } else {
    map_iter->second.emplace_back(watch_info);
  }
  return true;
}

/**
 * @brief 停止监控配置文件
 *
 * @param path 配置文件路径
 * @return true 停止成功
 * @return false 停止失败
 */
bool InotifyManager::goodbyeConfig(std::string_view path) {
  std::string path_str{path};
  auto watch_path_is_parent = getParentDir(path);

  auto [map_iter, vec_iter] = findWatchInfo(watch_path_is_parent, path_str);
  if (map_iter == map_.end() || vec_iter == map_iter->second.end()) {
    LOG(warn, "Path not watched: {}", path);
    return true;
  }

  ASSERT(map_iter != map_.end());
  ASSERT(vec_iter != map_iter->second.end());

  std::lock_guard<std::mutex> lock{mutex_};

  if (vec_iter->watch_fd != FD_INVALID) {
    // 只有数组第一个元素的 watch_fd 才是有效的，其余元素都是 FD_INVALID
    if (!inotify_->removeWatch(vec_iter->watch_fd)) {
      LOG(error, "Failed to remove watch: {}", watch_path_is_parent);
      return false;
    }
    LOG(info, "Inotify remove the watching: {}", watch_path_is_parent);
  }

  map_iter->second.erase(vec_iter);
  if (map_iter->second.empty()) {
    map_.erase(map_iter);
  }
  return true;
}

/**
 * @brief 初始化
 *
 * @param io_ctx IO 对象
 */
void InotifyManager::initManager(std::weak_ptr<io::IoIf> io_ctx) {
  ASSERT(inotify_ != nullptr);

  inotify_->setInotifyCb(InotifyCb{
      FUNCTION_LINE, [this](std::string_view name, uint32_t mask) { onInotifyEvent(name, mask); }});

  int inotify_fd = inotify_->getFd();
  if (inotify_fd == FD_INVALID) {
    throw EXCEPT("Failed to get inotify fd");
  }

  io_ctx_ = io_ctx;
  auto io_ctx_val = io_ctx_.lock();
  if (io_ctx_val == nullptr) {
    throw EXCEPT("MUST provide a io context");
  }

  watcher_ = io_ctx_val->addReadCb(
      inotify_fd, io::IoCb{FUNCTION_LINE, [this, inotify_fd]() {
                             constexpr int MAXSIZE_BUFFER{4096};
                             std::array<char, MAXSIZE_BUFFER> buffer{};
                             ssize_t len = read(inotify_fd, buffer.data(), buffer.size());
                             if (len > 0) {
                               inotify_->processInotify(buffer.data(), len);
                             }
                           }});
}

/**
 * @brief 停止监控管理器
 *
 */
void InotifyManager::destroyManager() {
  std::lock_guard<std::mutex> lock{mutex_};
  for (const auto &pair : map_) {
    for (const auto &elem : pair.second) {
      if (elem.watch_fd != FD_INVALID) {
        inotify_->removeWatch(elem.watch_fd);
      }
    }
  }
  map_.clear();
}

/**
 * @brief 查找迭代器
 *
 * @param parent_dir 父目录
 * @param path_str 配置文件
 * @note 函数内会加锁
 *
 * @return std::pair<InotifyManager::MapIterator, InotifyManager::VecIterator> 迭代器
 */
std::pair<InotifyManager::MapIterator, InotifyManager::VecIterator>
InotifyManager::findWatchInfo(std::string_view parent_dir, std::string_view path_str) {
  std::lock_guard<std::mutex> lock{mutex_};

  auto map_iter = map_.find(std::string{parent_dir});
  if (map_iter == map_.end()) {
    return {map_.end(), {}};
  }
  auto vec_iter =
      std::find_if(map_iter->second.begin(), map_iter->second.end(),
                   [&path_str](const WatchInfo &winfo) { return winfo.watch_config == path_str; });
  return {map_iter, vec_iter};
}

/**
 * @brief 获取上一层目录
 *
 * @param path 当前路径
 * @return std::string 上一层目录
 */
std::string InotifyManager::getParentDir(std::string_view path) {
  ASSERT(!path.empty());

  namespace fs = std::filesystem;
  fs::path p{path};
  std::string dir_path = p.parent_path().string();
  if (dir_path.empty()) {
    dir_path = ".";
  }
  return dir_path;
}

/**
 * @brief 管理器回调事件
 *
 * @param name 配置文件名称
 * @param mask 配置文件标识位
 */
void InotifyManager::onInotifyEvent(std::string_view name, uint32_t mask) {
  WatchInfo watch_info{};
  std::string path_str{name};
  auto watch_path_is_parent = getParentDir(name);
  auto [map_iter, vec_iter] = findWatchInfo(watch_path_is_parent, path_str);

  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (map_iter == map_.end() || vec_iter == map_iter->second.end()) {
      return;
    }

    ASSERT(vec_iter != map_iter->second.end());
    watch_info = *vec_iter;
  }
  if (watch_info.on_change) {
    watch_info.on_change(name, mask);
  }
}

} // namespace inotify
} // namespace hebpf
