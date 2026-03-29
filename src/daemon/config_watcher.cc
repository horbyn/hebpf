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
#include "src/inotify/inotify_manager.h"
// clang-format on

namespace hebpf {
namespace daemon {

ConfigWatcher::ConfigWatcher(std::weak_ptr<inotify::InotifyManagerIf> inotify_manager)
    : inotify_mngr_{inotify_manager} {}

ConfigWatcher::~ConfigWatcher() { clear(); }

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
                       if (weak_ptr.expired()) {
                         return true;
                       }
                       auto shared_ptr = weak_ptr.lock();
                       return shared_ptr == subscriber;
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

  if (config_path_ == config) {
    return;
  }
  config_path_ = std::string{config};

  auto inotify_mngr = inotify_mngr_.lock();
  if (!inotify_mngr) {
    throw EXCEPT("Inotify is invalid");
  }

  if (!inotify_mngr->watchConfig(
          config_path_,
          inotify::InotifyCb{FUNCTION_LINE, [this](std::string_view name, uint32_t mask) {
                               if (config_path_.find(name) != std::string::npos) {
                                 GLOBAL_LOG(trace, "Config file {} changed, mask is {:#x}", name,
                                            mask);
                                 loadConfig();
                               }
                             }})) {
    throw EXCEPT("Inotify add watch failed");
  }
  LOG(info, "Inotify watch {}", config_path_);

  // 初始加载配置
  loadConfig();
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

/**
 * @brief 清理资源
 *
 */
void ConfigWatcher::clear() {
  ASSERT(inotify_mngr_.lock());

  try {
    std::shared_ptr<inotify::InotifyManagerIf> inotify_mngr{};
    {
      std::lock_guard<std::mutex> lock{config_mutex_};
      inotify_mngr = inotify_mngr_.lock();
      if (!inotify_mngr) {
        return;
      }
    }

    inotify_mngr->goodbyeConfig(config_path_);
  } catch (const except::Exception &exc) {
    GLOBAL_LOG(error, "Failed to clear config {} on Inotify\nStackframe: {}", exc.what(),
               exc.stackFrame());
  } catch (const std::exception &exc) {
    GLOBAL_LOG(error, "Failed to clear config {} on Inotify", exc.what());
  }
}

} // namespace daemon
} // namespace hebpf
