// clang-format off
#include "debug_server.h"
#include <boost/beast/core/flat_buffer.hpp>
#include "src/common/common.h"
#include "src/callback/callback.h"
#include "src/common/assert.h"
#include "src/common/exception.h"
#include "src/io/io.h"
// clang-format on

namespace hebpf {
namespace debug_server {

namespace beast = boost::beast;
namespace http = beast::http;

using tcp = boost::asio::ip::tcp;

DebugServer::DebugServer(std::weak_ptr<io::IoIf> ioctx) : ioctx_{ioctx}, address_{}, port_{0} {
  running_.store(false);
  subscriber_list_ = std::make_shared<SubscriberList>();
}

/**
 * @brief 启动 Debug 服务器
 *
 * @note 启动不会初始化订阅者列表和 IO context，只会重新构造一个 HTTP 服务
 *
 * @param address 服务器地址
 * @param port 服务器端口
 *
 * @return true 成功；false 失败
 */
bool DebugServer::start(std::string_view address, uint16_t port) {
  setListen(address, port);

  auto io_ifptr = ioctx_.lock();
  if (io_ifptr == nullptr) {
    throw EXCEPT("IO context is invalid");
  }
  auto io_ptr = std::dynamic_pointer_cast<io::Io>(io_ifptr);
  if (io_ptr == nullptr) {
    throw EXCEPT("IO context is not io::Io");
  }

  auto &io_ctx = io_ptr->getIoContext();
  acceptor_ = std::make_unique<tcp::acceptor>(io_ctx);

  beast::error_code ec{};
  tcp::endpoint endpoint{boost::asio::ip::make_address(address_), port_};
  acceptor_->open(endpoint.protocol(), ec);
  if (ec) {
    LOG(error, "open(): {}", ec.message());
    return false;
  }
  acceptor_->set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    LOG(error, "set_option(): {}", ec.message());
    return false;
  }
  acceptor_->bind(endpoint, ec);
  if (ec) {
    LOG(error, "bind(): {}", ec.message());
    return false;
  }
  acceptor_->listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    LOG(error, "listen(): {}", ec.message());
    return false;
  }

  doAccept();
  LOG(info, "Debug server listening on {}:{}", address_, port_);
  return true;
}

/**
 * @brief 停止 Debug 服务端
 *
 * @note 停止不会清空订阅者列表和析构 IO context，只会析构旧 HTTP 服务
 */
void DebugServer::stop() {
  if (!running_.exchange(false)) {
    return;
  }
  address_.clear();
  port_ = 0;
  if (acceptor_ != nullptr) {
    beast::error_code ec{};
    acceptor_->close(ec);
  }
  LOG(info, "Debug server stopped");
}

/**
 * @brief 注册事件
 * @note 订阅者由外部模块（main()）负责，Debug Server 重启不清空已存在的订阅者列表
 *
 * @param subscriber 订阅者对象
 */
void DebugServer::attach(std::shared_ptr<subscribe::JsonSubscriberIf> subscriber) {
  if (subscriber == nullptr) {
    return;
  }

  auto old_list = std::atomic_load_explicit(&subscriber_list_, std::memory_order_acquire);
  auto new_list = std::make_shared<SubscriberList>();
  if (old_list) {
    *new_list = *old_list; // 深拷贝
  }

  auto iter = std::find(new_list->begin(), new_list->end(), subscriber);
  if (iter == new_list->end()) {
    new_list->push_back(std::move(subscriber));
  }

  std::atomic_store_explicit(&subscriber_list_, std::move(new_list), std::memory_order_release);
}

/**
 * @brief 取消注册事件
 * @note 订阅者由外部模块（main()）负责，Debug Server 重启不清空已存在的订阅者列表
 *
 * @param subscriber 订阅者对象
 */
void DebugServer::detach(std::shared_ptr<subscribe::JsonSubscriberIf> subscriber) {
  if (subscriber == nullptr) {
    return;
  }

  auto old_list = std::atomic_load_explicit(&subscriber_list_, std::memory_order_acquire);
  auto new_list = std::make_shared<SubscriberList>();
  if (old_list) {
    *new_list = *old_list;
  }

  auto iter = std::remove(new_list->begin(), new_list->end(), subscriber);
  if (iter != new_list->end()) {
    new_list->erase(iter, new_list->end());
    std::atomic_store_explicit(&subscriber_list_, std::move(new_list), std::memory_order_release);
  }
}

/**
 * @brief 通知订阅者事件发生，获取当前列表快照
 *
 * @return nlohmann::json JSON 配置
 */
nlohmann::json DebugServer::notify(void) {
  nlohmann::json root{};
  root["version"] = HEBPF_VERSION;

  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  root["build_time"] = std::ctime(&now);

  auto list = std::atomic_load_explicit(&subscriber_list_, std::memory_order_acquire);
  if (list) {
    for (const auto &sub : *list) {
      if (sub != nullptr) {
        sub->update(root);
      }
    }
  }
  return root;
}

/**
 * @brief 更新配置文件
 *
 * @param config 配置文件对象
 */
void DebugServer::update(const daemon::Configs &config) {
  // 1. 如果当前已启动，检查配置是否启用
  // 1-1. 配置启用，检查地址和端口是否都一致
  // 1-1-1. 一致，什么都不做
  // 1-1-2. 不一致，关闭当前服务，重新启动
  // 1-2. 配置禁用，关闭当前服务
  // 2. 如果当前未启动，检查配置是否启用
  // 2-1. 配置启用，启动服务
  // 2-2. 配置禁用，什么都不做

  auto current_enabled = config.getDebugServerEnabled();
  auto current_addr = config.getDebugServerAddr();
  auto current_port = config.getDebugServerPort();

  if (running_.load()) {
    if (current_enabled) {
      if (current_addr != address_ || current_port != port_) {
        stop();
        start(current_addr, current_port);
      } // end if(重启)
    } else {
      stop();
    } // end if-else(当前配置)
  } else {
    if (current_enabled) {
      start(current_addr, current_port);
    } // end if(启动)
  } // end if-else(当前状态)
}

/**
 * @brief 设置监听地址
 *
 * @param addr 监听地址
 * @param port 端口
 */
void DebugServer::setListen(std::string_view addr, uint16_t port) {
  ASSERT(!addr.empty());
  ASSERT(port > 0);
  address_ = std::string{addr};
  port_ = port;
}

/**
 * @brief 接收请求
 *
 */
void DebugServer::doAccept() {
  ASSERT(acceptor_ != nullptr);

  auto on_accept = common::Callback<void(beast::error_code, tcp::socket)>(
      FUNCTION_LINE, [self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
        if (ec) {
          if (self->running_.load()) {
            GLOBAL_LOG(error, "Accept error: {}", ec.message());
            self->doAccept();
          }
          return;
        }

        auto socket_ptr = std::make_shared<tcp::socket>(std::move(socket));
        auto req = std::make_shared<http::request<http::string_body>>();
        auto buffer = std::make_shared<beast::flat_buffer>();
        auto on_read = common::Callback<void(beast::error_code, size_t)>(
            FUNCTION_LINE, [self, socket_ptr, req, buffer](beast::error_code ec, size_t) mutable {
              if (ec) {
                GLOBAL_LOG(warn, "HTTP read error: {}", ec.message());
                return;
              }
              if (req->method() == http::verb::get && req->target() == "/api/debug") {
                auto json = self->notify();
                self->sendResponse(std::move(*socket_ptr), http::status::ok, json.dump(2));
              } else {
                self->sendResponse(std::move(*socket_ptr), http::status::not_found,
                                   R"({"error":"Not found"})");
              }
            });
        beast::http::async_read(*socket_ptr, *buffer, *req, std::move(on_read));
        self->doAccept();
      });

  acceptor_->async_accept(
      [on_accept = std::move(on_accept)](beast::error_code ec, tcp::socket socket) mutable {
        on_accept(ec, std::move(socket));
      });
}

/**
 * @brief 响应 HTTP 请求
 *
 * @param socket socket
 * @param status HTTP 状态
 * @param body HTTP 请求体
 */
void DebugServer::sendResponse(boost::asio::ip::tcp::socket socket,
                               boost::beast::http::status status, std::string_view body) {
  auto res = std::make_shared<http::response<http::string_body>>(status, 11); // HTTP/1.1
  res->set(http::field::server, "hebpf-debug");
  res->set(http::field::content_type, "application/json");
  res->body() = std::string{body};
  res->prepare_payload();

  auto socket_ptr = std::make_shared<tcp::socket>(std::move(socket));
  auto on_write = common::Callback<void(beast::error_code, size_t)>(
      FUNCTION_LINE, [socket_ptr, res_longtime = res](beast::error_code ec, size_t) mutable {
        (void)res_longtime; // 保证 response 的生命周期在回调中存在
        if (ec) {
          GLOBAL_LOG(warn, "HTTP write error: {}", ec.message());
        }
      });
  beast::http::async_write(*socket_ptr, *res, std::move(on_write));
}

} // namespace debug_server
} // namespace hebpf
