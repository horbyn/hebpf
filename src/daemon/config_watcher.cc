// clang-format off
#include "config_watcher.h"
#include <sys/inotify.h>
#include <chrono>
#include <filesystem>
#include <boost/asio.hpp>
#include "src/common/exception.h"
#include "src/inotify/inotify.h"
// clang-format on

namespace hebpf {
namespace daemon {

ConfigWatcher::ConfigWatcher(std::string_view config_path, signal::SignalIf &signal)
    : config_path_{config_path}, signal_{signal} {}

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
 * @brief 根据配置文件构造配置对象
 *
 * @param path 配置文件路径
 */
void ConfigWatcher::loadConfig(std::string_view path) {
  try {
    Configs cfg = Configs::loadFromConfig(path);
    std::lock_guard<std::mutex> lock{config_mutex_};
    if (cfg != last_config_) {
      last_config_ = cfg;
      if (callback_) {
        callback_(last_config_);
      }
    }
  } catch (const std::exception &exc) {
    GLOBAL_LOG(warn, "Failed to load config: {}", exc.what());
  }
}

/**
 * @brief watcher 事件循环
 *
 */
void ConfigWatcher::watchLoop() noexcept {
  try {

    inotify::Inotify inotify{};

    // 获取配置文件所在目录（用于监控目录，避免文件替换问题）
    namespace fs = std::filesystem;
    fs::path p(config_path_);
    std::string dir_path = p.parent_path().string();
    if (dir_path.empty()) {
      dir_path = ".";
    }
    LOG(info, "Inotify watch directory {}", dir_path);

    if (!inotify.addWatch(dir_path, IN_CLOSE_WRITE | IN_MOVED_TO)) {
      throw EXCEPT("Inotify add watch failed");
    }
    inotify.setInotifyCb([&](std::string_view name, uint32_t mask) {
      if (config_path_.find(name) != std::string::npos) {
        LOG(trace, "Config file {} changed, mask is {:#x}", name, mask);
        loadConfig(config_path_);
      }
    });

    int inotify_fd = inotify.getFd();
    if (inotify_fd == FD_INVALID) {
      throw EXCEPT("Failed to get inotify fd");
    }

    boost::asio::io_context io_ctx{};
    boost::asio::posix::stream_descriptor stream{io_ctx, inotify_fd};
    std::array<char, 4096> buffer{};
    std::function<void(const boost::system::error_code &, std::size_t)> read_handler =
        [&](const boost::system::error_code &ec, std::size_t length) {
          if (!ec) {
            inotify.processInotify(buffer.data(), length);
            stream.async_read_some(boost::asio::buffer(buffer), read_handler);
          }
        };
    stream.async_read_some(boost::asio::buffer(buffer), read_handler);

    boost::asio::posix::stream_descriptor stop_stream{io_ctx};
    int stop_fd = signal_.dupStopFd();
    if (stop_fd != FD_INVALID) {
      stop_stream.assign(stop_fd);
      uint64_t stop_val{};
      stop_stream.async_read_some(boost::asio::buffer(&stop_val, sizeof(stop_val)),
                                  [&](const boost::system::error_code &ec, std::size_t) {
                                    if (!ec) {
                                      io_ctx.stop();
                                      LOG(info, "Stop event received, exiting watch loop");
                                    }
                                  });
    }

    // 初始加载配置
    loadConfig(config_path_);

    io_ctx.run();

  } catch (const except::Exception &exc) {
    LOG(error, "\n[watcher error] {}\n[watcher stackframe]\n{}", exc.what(), exc.stackFrame());
  } catch (const std::exception &exc) {
    LOG(error, "\n[watcher error] {}", exc.what());
  }
}

} // namespace daemon
} // namespace hebpf
