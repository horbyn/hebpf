#pragma once

// clang-format off
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include "boost/asio/ip/tcp.hpp"
#include "boost/beast/http.hpp"
#include "src/daemon/configs.hpp"
#include "src/log/logger.h"
#include "src/io/io_if.h"
#include "src/subscribe/json_publisher_if.h"
#include "src/subscribe/yaml_subscriber_if.h"
// clang-format on

namespace hebpf {
namespace debug_server {

using SubscriberList = std::vector<std::shared_ptr<subscribe::JsonSubscriberIf>>;

class DebugServer : public subscribe::JsonPublisherIf,
                    public subscribe::YamlSubscriberIf,
                    public log::Loggable<log::Id::debug_server>,
                    public std::enable_shared_from_this<DebugServer> {
public:
  explicit DebugServer(std::weak_ptr<io::IoIf> ioctx);

  bool start(std::string_view address = daemon::DEFAULT_DBGSERV_ADDR,
             uint16_t port = daemon::DEFAULT_DBGSERV_PORT);
  void stop();

  void attach(std::shared_ptr<subscribe::JsonSubscriberIf> subscriber) override;
  void detach(std::shared_ptr<subscribe::JsonSubscriberIf> subscriber) override;
  nlohmann::json notify(void) override;
  void update(const daemon::Configs &config) override;

private:
  void setListen(std::string_view addr, uint16_t port);
  void doAccept();
  void sendResponse(boost::asio::ip::tcp::socket socket, boost::beast::http::status status,
                    std::string_view body);

  std::weak_ptr<io::IoIf> ioctx_;
  std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
  std::string address_;
  uint16_t port_;

  std::atomic<bool> running_;
  std::shared_ptr<SubscriberList> subscriber_list_; // RCU 保护
};

} // namespace debug_server
} // namespace hebpf
