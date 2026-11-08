#pragma once

// clang-format off
#include <mutex>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"
#include "src/ebpf/ebpf_if.h"
#include "src/services/top/top.skel.h"
#include "top_common.h"
// clang-format on

namespace hebpf {
namespace services {
namespace top {

class Top : public ebpf::EbpfSkelIf<top_bpf> {
public:
  explicit Top(std::unique_ptr<top_bpf> skel = {});

  std::string getName() const;
  void load() override;
  int getRunningFd(daemon::HookType hook_type) const override;
  nlohmann::json getPrometheusStatus() override;

private:
  using KData = std::vector<struct top_cpu_time>;

  bool readFromDatapath(KData &kernel_data) const;
  bool CacheDatapath();

  mutable std::mutex mutex_;
  int map_fd_;
  int n_cpus_;
  KData last_;    // 上次采样的累计值
  KData pending_; // 累积增量，等待 Prometheus 消费
};

} // namespace top
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
