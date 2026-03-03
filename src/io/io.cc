// clang-format off
#include "io.h"
#include <pthread.h>
#include <unistd.h>
#include <memory>
#include <vector>
#include <boost/asio.hpp>
#include "src/common/exception.h"
#include "src/fd/fd.h"
#include "src/thread/thread.h"
// clang-format on

namespace hebpf {
namespace io {

constexpr std::string_view NAME_IOCONTEXT{"io-context"};

/**
 * @brief 封装 boost::asio::posix::descriptor_base
 *
 * @note 这个对象会用在异步 IO 中，有可能是外部模块（具体的对象）和回调函数两处来引用，所以需要用
 * std::shared_ptr 来共享本对象的生命周期，出于这个目的此处继承 std::enable_shared_from_this
 */
class DescriptorBase : public std::enable_shared_from_this<DescriptorBase> {
public:
  DescriptorBase(boost::asio::io_context &io_ctx, int fd, io::IoCb handler)
      : stream_{io_ctx}, fd_{fd}, handler_{std::move(handler)} {
    int dup_fd = dup(fd);
    if (dup_fd == -1) {
      throw EXCEPT("dup failed", true);
    }
    stream_.assign(dup_fd);
  }

  /**
   * @brief 启动异步事件
   *
   */
  void start() { async_wait(); }

private:
  /**
   * @brief 启动异步事件
   *
   */
  void async_wait() {
    stream_.async_wait(boost::asio::posix::stream_descriptor::wait_read,
                       [weak_self = weak_from_this()](const boost::system::error_code &ec) {
                         if (auto self = weak_self.lock()) {
                           if (!ec && self->handler_) {
                             self->handler_();
                             self->async_wait();
                           }
                         }
                       });
  }

  boost::asio::posix::stream_descriptor stream_;
  int fd_;
  io::IoCb handler_;
};

/**
 * @brief 封装 boost::asio::io_context
 *
 */
class Io::IoContext {
  friend class Io;

public:
  IoContext() : work_guard_(boost::asio::make_work_guard(io_ctx_)) {
    worker_thread_ = std::make_unique<thread::Thread>();
    worker_thread_->setName(NAME_IOCONTEXT);
    worker_thread_->start([this] {
      try {
        io_ctx_.run();
      } catch (std::exception &e) {
        GLOBAL_LOG(error, "io_context run error: {}", e.what());
      }
    });
  }

  ~IoContext() {
    stopped_ = true;
    work_guard_.reset();
    io_ctx_.stop();
    if (worker_thread_->joinable()) {
      worker_thread_->join();
    }
  }

private:
  boost::asio::io_context io_ctx_;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
  std::unique_ptr<thread::ThreadIf> worker_thread_;
  std::atomic<bool> stopped_{false};
};

Io::Io() : impl_{std::make_unique<IoContext>()} {}

Io::~Io() {}

/**
 * @brief 注册异步读事件
 *
 * @param fd 事件对应的 fd
 * @param callback 响应时间的回调函数
 * @return std::shared_ptr<void> 与异步 IO 共享的对象
 */
std::shared_ptr<void> Io::addReadCb(int fd, IoCb callback) {
  auto watcher = std::make_shared<DescriptorBase>(impl_->io_ctx_, fd, std::move(callback));
  impl_->io_ctx_.post([watcher] { watcher->start(); });
  return watcher;
}

} // namespace io
} // namespace hebpf
