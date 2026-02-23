// clang-format off
#include "config_watcher.h"
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <thread>
#include <unistd.h>
#include "src/common/exception.h"
// clang-format on

namespace hebpf {
namespace daemon {

namespace {

/**
 * @brief 根据配置文件构造配置对象
 *
 * @param path 配置文件路径
 * @return std::optional<Configs> 配置对象
 */
std::optional<Configs> loadConfig(std::string_view path) {
  try {
    Configs cfg = Configs::loadFromConfig(path);
    return cfg;
  } catch (const std::exception &exc) {
    GLOBAL_LOG(warn, "Failed to load config: {}", exc.what());
    return std::nullopt;
  }
}
} // anonymous namespace

ConfigWatcher::ConfigWatcher(std::string_view config_path) : config_path_{config_path} {}

ConfigWatcher::~ConfigWatcher() { stop(); }

/**
 * @brief 启动 watcher
 *
 * @param callback 回调函数
 */
void ConfigWatcher::start(ConfigChangeCallback callback) {
  if (running_.load()) {
    return;
  }
  callback_ = std::move(callback);
  running_.store(true);
  watch_thread_ = std::thread(&ConfigWatcher::watchLoop, this);
}

/**
 * @brief 停止 watcher
 *
 */
void ConfigWatcher::stop() {
  if (!running_.load()) {
    return;
  }
  running_.store(false);
  if (watch_thread_.joinable()) {
    watch_thread_.join();
  }
}

/**
 * @brief watcher 事件循环
 *
 */
void ConfigWatcher::watchLoop() noexcept {
  try {

    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd == -1) {
      throw EXCEPT("inotify_init() failed", true);
    }

    // 获取配置文件所在目录（用于监控目录，避免文件替换问题）
    namespace fs = std::filesystem;
    fs::path p(config_path_);
    std::string dir_path = p.parent_path().string();
    if (dir_path.empty()) {
      dir_path = ".";
    }
    LOG(info, "Inotify watch directory {}", dir_path);

    int wd = inotify_add_watch(inotify_fd, dir_path.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO);
    if (wd == -1) {
      close(inotify_fd);
      throw EXCEPT("inotify_add_watch() failed", true);
    }

    // TODO: 封装 epoll
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
      close(inotify_fd);
      throw EXCEPT("epoll_create() failed", true);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = inotify_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, inotify_fd, &ev) == -1) {
      close(inotify_fd);
      close(epoll_fd);
      throw EXCEPT("epoll_ctl() add inotify failed", true);
    }

    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS]{};

    // 初始加载配置
    if (auto cfg = loadConfig(config_path_)) {
      std::lock_guard<std::mutex> lock(config_mutex_);
      last_config_ = *cfg;
      if (callback_) {
        callback_(*cfg);
      }
    }

    constexpr int MIN_TIMEOUT = 1000; // 1秒超时
    while (running_) {
      int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, MIN_TIMEOUT);
      if (nfds == -1) {
        if (errno == EINTR) {
          continue;
        }
        LOG(error, "epoll_wait() error: {}", strerror(errno));
        break;
      }

      for (int i = 0; i < nfds; ++i) {
        if (events[i].data.fd == inotify_fd) {
          char buffer[4096]{};
          ssize_t len = read(inotify_fd, buffer, sizeof(buffer));
          if (len <= 0) {
            continue;
          }

          const struct inotify_event *event;
          for (char *ptr = buffer; ptr < buffer + len;) {
            event = reinterpret_cast<const struct inotify_event *>(ptr);
            if (event->len > 0 && config_path_.find(event->name) != std::string::npos) {
              LOG(trace, "Config file {} changed, mask is {:#x}", event->name, event->mask);

              if (auto new_cfg = loadConfig(config_path_)) {
                std::lock_guard<std::mutex> lock(config_mutex_);
                if (*new_cfg != last_config_) {
                  last_config_ = std::move(*new_cfg);
                  if (callback_) {
                    callback_(last_config_);
                  }
                }
              }
            }
            ptr += sizeof(struct inotify_event) + event->len;
          }
        }
      }
    }

    close(inotify_fd);
    close(epoll_fd);
  } catch (const except::Exception &exc) {
    LOG(error, "[watcher error] {}\n[watcher stackframe]\n{}", exc.what(), exc.stackFrame());
  } catch (const std::exception &exc) {
    LOG(error, "[watcher error] {}", exc.what());
  }
}

} // namespace daemon
} // namespace hebpf
