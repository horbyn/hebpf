#pragma once

// clang-format off
#include <memory>
#include "src/data/queue_if.h"
#include "src/ebpf/ebpf_if.h"
#include "src/ebpf/ringbuffer_if.h"
#include "src/services/iotrace/iotrace.skel.h"
#include "iotrace_comm.h"
// clang-format on

namespace hebpf {
namespace services {
namespace iotrace {

class Iotrace : public ebpf::EbpfSkelIf<iotrace_bpf> {
public:
  explicit Iotrace(std::unique_ptr<iotrace_bpf> skel = {});
  ~Iotrace();

  std::string getName() const override;
  void load() override;
  int getRunningFd(daemon::HookType hook_type) const override;
  nlohmann::json getPrometheusStatus() override;
  [[nodiscard]] bool onConfigUpdate(const nlohmann::json &config) override;

private:
  int onEvent(void *data, size_t data_sz);

  std::unique_ptr<ebpf::RingbufferIf> ringbuff_;
  std::unique_ptr<QueueIf<hebpf_io_event>> records_;
};

} // namespace iotrace
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
