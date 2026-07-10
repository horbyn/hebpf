#pragma once

// clang-format off
#include <string_view>
#include "src/log/logger.h"
#include "src/callback/callback.h"
#include "src/ebpf/pinned_ringbuffer_map.h"
#include "src/loki/loki_client_if.h"
#include "src/subscribe/yaml_subscriber_if.h"
#include "klog.bpf.h"
// clang-format on

namespace hebpf {
namespace services {
namespace klog {

class Klog : public log::Loggable<log::Id::ebpf>, public subscribe::YamlSubscriberIf {
public:
  explicit Klog(std::weak_ptr<io::IoIf> io_ctx, std::string_view pin_path = PATH_PIN,
                size_t map_size = KLOG_BUFFER_SIZE, std::string_view map_name = KLOG_BUFFER_NAME);

  Klog(const Klog &) = delete;
  Klog &operator=(const Klog &) = delete;

  Klog(Klog &&) noexcept = default;
  Klog &operator=(Klog &&) noexcept = default;

  void update(const daemon::Configs &config) override;

  void unpin();

private:
  int logEvent(void *data, size_t size);

  std::unique_ptr<ebpf::RingBufferMap> mgr_;
  std::unique_ptr<ebpf::Ringbuffer> ringbuf_;
  std::weak_ptr<io::IoIf> ioctx_;
  std::shared_ptr<loki::LokiClientIf> loki_client_;
  std::mutex loki_mutex_;
};

} // namespace klog
} // namespace services
} // namespace hebpf
