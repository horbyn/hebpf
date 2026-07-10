// clang-format off
#include "loki_client.h"
#include "boost/asio/connect.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/http.hpp"
#include "boost/beast/version.hpp"
#include <chrono>
#include <sstream>
#include "src/common/exception.h"
#include "src/io/io.h"
#include "src/thread/thread.h"
#include "loki_streams.h"
// clang-format on

namespace hebpf {
namespace loki {

/**
 * @brief LokiClient 构造函数
 *
 * @param ioctx              IoIf 对象
 * @param host               Loki HTTP 服务端地址
 * @param port               Loki HTTP 服务端端口
 * @param path               Loki HTTP 推送端点，例如 loki/api/v1/push
 * @param batch_size         每批次最大日志条数
 * @param flush_interval_sec 定时刷新间隔（秒）
 */
LokiClient::LokiClient(std::weak_ptr<io::IoIf> ioctx, std::string_view host, uint16_t port,
                       std::string_view path, int batch_size, int flush_interval_sec)
    : ioctx_{ioctx}, host_{std::string{host}}, port_{port}, path_{std::string{path}},
      batch_size_{batch_size}, flush_interval_sec_{flush_interval_sec} {
  worker_ = std::make_unique<thread::Thread>();
  worker_->setName(NAME_LOKI);
  worker_->start(std::bind(&LokiClient::senderThread, this));
}

LokiClient::~LokiClient() {
  stop();
  if (worker_ != nullptr && worker_->joinable()) {
    worker_->join();
    worker_.reset();
  }
}

/**
 * @brief 推送一条日志
 * @param line         日志内容
 * @param extra_labels 本条日志附加标签（会覆盖全局同名标签）
 */
void LokiClient::push(std::string_view line,
                      const std::map<std::string, std::string> &extra_labels) {
  if (!running_.load()) {
    LOG(warn, "Loki client is stopped, dropping log");
    return;
  }
  // 获取当前 Unix 纳秒时间戳（以纳秒为单位）
  auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
  LogEntry entry{};
  entry.line = std::string{line};
  entry.labels = extra_labels;
  entry.timestamp = std::chrono::nanoseconds(now_ns);

  {
    std::lock_guard<std::mutex> lock{mutex_};
    buffer_.push_back(std::move(entry));
    if (static_cast<int>(buffer_.size()) >= batch_size_) {
      cv_.notify_one();
    }
  }
}

/**
 * @brief 强制立即发送缓冲区中所有日志
 */
void LokiClient::flush() { cv_.notify_one(); }

/**
 * @brief 停止后台发送线程
 */
void LokiClient::stop() {
  running_.store(false);
  cv_.notify_all();
}

/**
 * @brief loki 服务端推送线程
 *
 */
void LokiClient::senderThread() {
  while (running_.load()) {
    std::unique_lock<std::mutex> lock{mutex_};
    cv_.wait_for(lock, std::chrono::seconds(flush_interval_sec_), [this] {
      return !running_.load() || static_cast<int>(buffer_.size()) >= batch_size_;
    });

    if (buffer_.empty()) {
      continue;
    }

    std::vector<LogEntry> to_send{};
    to_send.swap(buffer_);
    lock.unlock();

    if (!to_send.empty()) {
      nlohmann::json payload = buildPayload(to_send);
      bool ok = httpPost(payload);
      if (!ok) {
        LOG(error, "Failed to push logs to Loki, discarding {} entries", to_send.size());
      } else {
        LOG(debug, "Pushed {} logs to Loki", to_send.size());
      }
    }
  }
}

/**
 * @brief 构造 Loki 推送请求体
 */
nlohmann::json LokiClient::buildPayload(const std::vector<LogEntry> &entries) const {
  // 按标签分组（标签序列化为字符串作为临时 key）
  std::map<std::string, LokiStreamEntry> groups{};

  for (const auto &entry : entries) {
    std::map<std::string, std::string> labels{};
    std::string key{};
    for (auto iter = entry.labels.begin(); iter != entry.labels.end(); ++iter) {
      labels[iter->first] = iter->second;
      if (iter != entry.labels.begin()) {
        key += ",";
      }
      key += iter->first + "=" + iter->second;
    }

    auto &grp = groups[key];
    if (grp.stream.empty()) {
      grp.stream = std::move(labels);
    }
    grp.values.emplace_back(std::to_string(entry.timestamp.count()), entry.line);
  }

  LokiStreams result{};
  for (auto &kv : groups) {
    result.appendStream(std::move(kv.second));
  }

  return result;
}

/**
 * @brief 通过 HTTP POST 发送数据
 * @return true 成功，false 失败
 */
bool LokiClient::httpPost(const nlohmann::json &payload) {
  try {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;

    // 创建 io_context 和 resolver
    auto io_ifptr = ioctx_.lock();
    if (io_ifptr == nullptr) {
      throw EXCEPT("IO context is invalid");
    }
    auto io_ptr = std::dynamic_pointer_cast<io::Io>(io_ifptr);
    if (io_ptr == nullptr) {
      throw EXCEPT("IO context is not io::Io");
    }
    auto &io_ctx = io_ptr->getIoContext();
    tcp::resolver resolver{io_ctx};
    beast::tcp_stream stream{io_ctx};

    // 解析主机并连接
    auto const results = resolver.resolve(host_, std::to_string(port_));
    stream.connect(results);

    // 构建 HTTP 请求
    auto url = fmt::format("http://{}:{}/{}", host_, std::to_string(port_), path_);
    std::string body = payload.dump();
    LOG(debug, "Sending to {}:\n{}", url, body);
    http::request<http::string_body> req{http::verb::post, url, 11};
    req.set(http::field::host, host_);
    req.set(http::field::content_type, "application/json");
    req.set(http::field::user_agent, "hebpf/1.0");
    req.body() = body;
    req.prepare_payload(); // 自动计算 Content-Length

    // 发送请求
    http::write(stream, req);

    // 接收响应
    beast::flat_buffer buffer{};
    http::response<http::string_body> res{};
    http::read(stream, buffer, res);

    // 关闭连接
    beast::error_code ec{};
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    // 检查状态码
    int status = res.result_int();
    if (status >= 200 && status < 300) {
      return true;
    } else {
      LOG(error, "Loki returned HTTP {}", status);
      return false;
    }

  } catch (const std::exception &e) {
    LOG(error, "Boost HTTP exception: {}", e.what());
    return false;
  }
}

} // namespace loki
} // namespace hebpf
