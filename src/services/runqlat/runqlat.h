#pragma once

// clang-format off
#include <deque>
#include <mutex>
#include "src/ebpf/ebpf_if.h"
#include "src/ebpf/ringbuffer_if.h"
#include "src/services/runqlat/runqlat.skel.h"
#include "runqlat_comm.h"
// clang-format on

namespace hebpf {
namespace services {
namespace runqlat {

constexpr uint64_t MAX_CAPACITY{10240};

class Runqlat : public ebpf::EbpfSkelIf<runqlat_bpf> {
public:
  explicit Runqlat(std::unique_ptr<runqlat_bpf> skel = {});

  std::string getName() const override;
  void load() override;
  int getRunningFd(daemon::HookType hook_type) const override;
  nlohmann::json getDebugStatus() const override;
  nlohmann::json getPrometheusStatus() override;
  [[nodiscard]] bool onConfigUpdate(const nlohmann::json &config) override;

private:
  static std::string formatLatencyNs(uint64_t ns);
  static std::string safeComm(const char (&comm)[MAX_PROG_NAME]);

  int onEvent(void *data, size_t data_sz);
  void updateClockOffset();
  std::string formatBeijingTime(uint64_t monotonic_ns) const;
  std::string formatMonotonicSec(uint64_t monotonic_ns) const;

  std::unique_ptr<ebpf::RingbufferIf> ringbuff_;
  uint64_t clock_offset_ns_;
  mutable std::mutex mutex_;
  std::deque<rq_event> records_;
};

} // namespace runqlat
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
