#pragma once

// clang-format off
#include <mutex>
#include <map>
#include "src/ebpf/ebpf_if.h"
#include "src/ebpf/ringbuffer_if.h"
#include "src/services/examples/usermode/example_usermode.bpf.h"
#include "src/services/examples/usermode/example_usermode.skel.h"
// clang-format on

namespace hebpf {
namespace services {
namespace examples {

constexpr size_t MAX_EVENTS = 10240;

class ExampleUsermodeEbpf : public ebpf::EbpfSkelIf<struct example_usermode_bpf> {
public:
  explicit ExampleUsermodeEbpf(std::unique_ptr<example_usermode_bpf> skel = {});

  std::string getName() const override;
  void load() override;
  bool start(std::weak_ptr<io::IoIf> io_ctx = {}) override;
  void stop() override;
  nlohmann::json getPrometheusStatus() override;

private:
  int onEvent(void *data, size_t data_sz);

  std::unique_ptr<ebpf::RingbufferIf> ringbuff_;
  mutable std::mutex mutex_;
  uint64_t count_{0};
};

} // namespace examples
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
