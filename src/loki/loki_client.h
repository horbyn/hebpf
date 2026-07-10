#pragma once

// clang-format off
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>
#include "nlohmann/json.hpp"
#include "src/io/io_if.h"
#include "src/log/logger.h"
#include "src/thread/thread_if.h"
#include "loki_client_if.h"
// clang-format on

namespace hebpf {
namespace loki {

constexpr std::string_view NAME_LOKI{"loki"};

class LokiClient : public log::Loggable<log::Id::loki>, public LokiClientIf {
public:
  LokiClient(std::weak_ptr<io::IoIf> ioctx, std::string_view host, uint16_t port,
             std::string_view path, int batch_size, int flush_interval_sec);
  ~LokiClient();

  void push(std::string_view line, const std::map<std::string, std::string> &extra_labels) override;
  void flush() override;
  void stop() override;

private:
  void senderThread();

  struct LogEntry {
    std::string line;
    std::map<std::string, std::string> labels;
    std::chrono::nanoseconds timestamp;
  };
  nlohmann::json buildPayload(const std::vector<LogEntry> &entries) const;
  bool httpPost(const nlohmann::json &payload);

  std::weak_ptr<io::IoIf> ioctx_;
  std::string host_;
  uint16_t port_;
  std::string path_;
  int batch_size_;
  int flush_interval_sec_;

  mutable std::mutex mutex_;
  std::vector<LogEntry> buffer_;
  std::condition_variable cv_;
  std::atomic<bool> running_{true};
  std::unique_ptr<thread::ThreadIf> worker_;
};

} // namespace loki
} // namespace hebpf
