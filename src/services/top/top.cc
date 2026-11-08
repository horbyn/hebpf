// clang-format off
#include "top.h"
#include <bpf/bpf.h>
#include "src/common/assert.h"
#include "src/common/exception.h"
#include "src/data/kernel_data.h"
#include "src/data/metrics.h"
#include "generated/ebpf_prog_map.h"
// clang-format on

namespace {

/**
 * @brief 安全计算百分比
 *
 * @param part 部分
 * @param total 总数
 * @return double 百分比
 */
double pct(unsigned long long part, unsigned long long total) {
  return total == 0 ? 0.0 : static_cast<double>(part) * 100.0 / static_cast<double>(total);
}

/**
 * @brief 累计值差分（内核累计值理论上只增不减，回退时用 0 兜底）
 *
 * @param cur 当前值
 * @param prev 上一次记录值
 * @return uint64_t 差值
 */
uint64_t diff(unsigned long long cur, unsigned long long prev) {
  return cur >= prev ? static_cast<uint64_t>(cur - prev) : static_cast<uint64_t>(0);
}

} // namespace

namespace hebpf {
namespace services {
namespace top {

Top::Top(std::unique_ptr<top_bpf> skel)
    : ebpf::EbpfSkelIf<top_bpf>{std::move(skel)}, map_fd_{FD_INVALID}, n_cpus_{0} {
  n_cpus_ = libbpf_num_possible_cpus();
  if (n_cpus_ <= 0) {
    throw EXCEPT("Cannot get number of CPUs");
  }
  last_.assign(static_cast<std::size_t>(n_cpus_), top_cpu_time{});
  pending_.assign(static_cast<std::size_t>(n_cpus_), top_cpu_time{});
}

/**
 * @brief 获取 top 监控程序的名称
 *
 * @return std::string 程序名称
 */
std::string Top::getName() const { return std::string{TOP_Sched_Switch}; }

/**
 * @brief 加载 top 监控程序
 *
 */
void Top::load() {
  ebpf::EbpfSkelIf<top_bpf>::load();

  map_fd_ = bpf_map__fd(skel_->maps.hebpf_top_cputm);
  if (map_fd_ < 0) {
    throw EXCEPT("Failed to get the fd of eBPF map");
  }

  if (!readFromDatapath(last_)) {
    LOG(warn, "Cannot get the snapshots of on-CPU time on loading, which may cause some deviation");
  }

  if (last_.empty()) {
    LOG(warn, "Cannot get the first on-CPU time, so this snapshot will be empty");
  }
}

/**
 * @brief 获取 top 监控程序 fd
 * @param hook_type 绑定类型
 *
 * @return int fd
 */
int Top::getRunningFd([[maybe_unused]] daemon::HookType hook_type) const {
  return this->skel_ != nullptr ? bpf_program__fd(this->skel_->progs.hebpf_top_sched_switch)
                                : FD_INVALID;
}

/**
 * @brief on-CPU 时间从内核态读取
 * @param kernel_data 结果集（保存结果）
 *
 * @return true 成功
 * @return false 失败
 */
bool Top::readFromDatapath(Top::KData &kernel_data) const {
  __u32 key = INDEX_CPU_USAGE;
  return bpf_map_lookup_elem(map_fd_, &key, kernel_data.data()) == 0;
}

/**
 * @brief 缓存内核态数据
 * @note 缓存到 pending_
 *
 * @return true 成功
 * @return false 失败
 */
bool Top::CacheDatapath() {
  ASSERT(map_fd_ >= 0);

  KData current(static_cast<std::size_t>(n_cpus_), top_cpu_time{});
  if (!readFromDatapath(current)) {
    return false;
  }

  std::lock_guard<std::mutex> lock{mutex_};
  for (int i = 0; i < n_cpus_; ++i) {
    const auto &cur = current[i];
    const auto &prev = last_[i];

    pending_[i].user_ns += diff(cur.user_ns, prev.user_ns);
    pending_[i].system_ns += diff(cur.system_ns, prev.system_ns);
    pending_[i].idle_ns += diff(cur.idle_ns, prev.idle_ns);

    last_[i] = cur;
  } // end for()

  return true;
}

/**
 * @brief 获取 run queue 内核态数据对接 Prometheus
 *
 * @return nlohmann::json 内核数据对象
 */
nlohmann::json Top::getPrometheusStatus() {
  Metrics metrics{};
  metrics.service_ = getName();

  if (map_fd_ < 0) {
    // 还未加载过 eBPF 程序
    return metrics;
  }
  if (!CacheDatapath()) {
    return metrics;
  }

  decltype(pending_) copy{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    copy = pending_;
    std::fill(pending_.begin(), pending_.end(), top_cpu_time{});
  }

  for (int idx = 0; idx < n_cpus_; ++idx) {
    const auto &delta = copy[idx];
    const unsigned long long sum = delta.user_ns + delta.system_ns + delta.idle_ns;

    if (sum == 0) {
      continue; // 该 CPU 在本采样周期内没有任何增量，跳过
    }

    const KernelData::KernelDataMap labels{
        {"cpu", std::to_string(idx)},
    };

    metrics.metrics_.push_back(
        KernelData{MetricType::GAUGE, "hebpf_top_user_percent", pct(delta.user_ns, sum), labels});
    metrics.metrics_.push_back(KernelData{MetricType::GAUGE, "hebpf_top_system_percent",
                                          pct(delta.system_ns, sum), labels});
    metrics.metrics_.push_back(
        KernelData{MetricType::GAUGE, "hebpf_top_idle_percent", pct(delta.idle_ns, sum), labels});

  } // end for()

  return metrics;
}

} // namespace top
} // namespace services
} // namespace hebpf

/**
 * @brief 动态库导出接口：用来返回 eBPF 对象
 *
 * @return std::unique_ptr<hebpf::ebpf::EbpfIf> 对象
 */
extern "C" std::unique_ptr<hebpf::ebpf::EbpfIf> create_service() {
  return std::make_unique<hebpf::services::top::Top>();
}
