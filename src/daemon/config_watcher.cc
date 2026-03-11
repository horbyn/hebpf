// clang-format off
#include "config_watcher.h"
#include <pthread.h>
#include <sys/inotify.h>
#include <chrono>
#include <filesystem>
#include <boost/asio.hpp>
#include "src/common/assert.h"
#include "src/common/common.h"
#include "src/common/exception.h"
#include "src/inotify/inotify.h"
// clang-format on

namespace hebpf {
namespace daemon {

ConfigWatcher::ConfigWatcher(std::unique_ptr<inotify::InotifyIf> inotify)
    : inotify_{std::move(inotify)} {}

/**
 * @brief 注册事件
 *
 * @param subscriber 订阅者对象
 */
void ConfigWatcher::attach(std::shared_ptr<subscribe::SubscriberIf> subscriber) {
  std::lock_guard<std::mutex> lock{subscribers_mutex_};
  subscribers_.push_back(subscriber);
  if (!config_path_.empty()) {
    Configs current{};
    {
      std::lock_guard<std::mutex> lock2{config_mutex_};
      current = last_config_;
    }
    subscriber->update(current);
  }
}

/**
 * @brief 取消注册事件
 *
 * @param subscriber 订阅者对象
 */
void ConfigWatcher::detach(std::shared_ptr<subscribe::SubscriberIf> subscriber) {
  std::lock_guard<std::mutex> lock{subscribers_mutex_};
  subscribers_.erase(
      std::remove_if(subscribers_.begin(), subscribers_.end(),
                     [&subscriber](const std::weak_ptr<subscribe::SubscriberIf> &weak_ptr) {
                       auto shared_ptr = weak_ptr.lock();
                       return shared_ptr != nullptr && shared_ptr == subscriber;
                     }),
      subscribers_.end());
}

/**
 * @brief 通知订阅者事件发生
 *
 */
void ConfigWatcher::notify() {
  std::vector<std::shared_ptr<subscribe::SubscriberIf>> active{};
  {
    std::lock_guard<std::mutex> lock{subscribers_mutex_};
    for (auto &weak_ptr : subscribers_) {
      if (auto shared_ptr = weak_ptr.lock()) {
        active.push_back(shared_ptr);
      }
    }
  }
  Configs current{};
  {
    std::lock_guard<std::mutex> lock{config_mutex_};
    current = last_config_;
  }
  for (auto &subscriber : active) {
    subscriber->update(current);
  }
}

/**
 * @brief 注册配置文件变化事件
 *
 * @param config 配置文件路径
 */
void ConfigWatcher::startWatching(std::string_view config) {
  ASSERT(inotify_ != nullptr);

  if (config_path_ == config) {
    return;
  }
  config_path_ = std::string{config};

  // 获取配置文件所在目录（用于监控目录，避免文件替换问题）
  namespace fs = std::filesystem;
  fs::path p(config_path_);
  std::string dir_path = p.parent_path().string();
  if (dir_path.empty()) {
    dir_path = ".";
  }

  if (!inotify_->addWatch(dir_path, IN_CLOSE_WRITE | IN_MOVED_TO)) {
    throw EXCEPT("Inotify add watch failed");
  }
  inotify_->setInotifyCb(
      inotify::InotifyCb{FUNCTION_LINE, [this](std::string_view name, uint32_t mask) {
                           if (config_path_.find(name) != std::string::npos) {
                             GLOBAL_LOG(trace, "Config file {} changed, mask is {:#x}", name, mask);
                             loadConfig();
                           }
                         }});
  LOG(info, "Inotify watch directory {}", dir_path);

  int inotify_fd = inotify_->getFd();
  if (inotify_fd == FD_INVALID) {
    throw EXCEPT("Failed to get inotify fd");
  }

  auto io_ctx_val = io_ctx_.lock();
  if (io_ctx_val == nullptr) {
    throw EXCEPT("MUST provide a io context");
  }

  inotify_watcher_ = io_ctx_val->addReadCb(
      inotify_fd, io::IoCb{FUNCTION_LINE, [this, inotify_fd]() {
                             constexpr int MAXSIZE_BUFFER{4096};
                             std::array<char, MAXSIZE_BUFFER> buffer{};
                             ssize_t len = read(inotify_fd, buffer.data(), buffer.size());
                             if (len > 0) {
                               inotify_->processInotify(buffer.data(), len);
                             }
                           }});

  // 初始加载配置
  loadConfig();
}

/**
 * @brief 设置 io 模块
 *
 * @param io_ctx io 对象
 * @return true 设置成功
 * @return false 设置失败
 */
bool ConfigWatcher::setIoContext(std::weak_ptr<io::IoIf> io_ctx) {
  io_ctx_ = io_ctx;
  return static_cast<bool>(io_ctx_.lock() != nullptr);
}

/**
 * @brief 根据配置文件构造配置对象
 *
 */
void ConfigWatcher::loadConfig() {
  try {
    Configs cfg = Configs::loadFromConfig(config_path_);
    bool changed = false;
    {
      std::lock_guard<std::mutex> lock{config_mutex_};
      if (cfg != last_config_) {
        last_config_ = cfg;
        changed = true;
      }
    }
    if (changed) {
      notify();
    }
  } catch (const std::exception &exc) {
    GLOBAL_LOG(warn, "Failed to load config: {}", exc.what());
  }
}

} // namespace daemon
} // namespace hebpf
