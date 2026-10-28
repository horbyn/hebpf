// clang-format off
#include "runqlat.h"
#include <bpf/bpf.h>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include "src/common/common.h"
#include "src/common/exception.h"
#include "src/data/kernel_data.h"
#include "src/data/metrics.h"
#include "src/ebpf/ringbuffer.h"
#include "src/fd/fd.h"
#include "generated/ebpf_prog_map.h"
#include "runqlat_conf.h"
// clang-format on

namespace {

constexpr uint64_t ns_per_sec = 1'000'000'000ULL;
constexpr uint64_t beijing_offset_ns = 8ULL * 3600ULL * ns_per_sec; // UTC+8
constexpr std::string_view JKEY_RUNQ_COMM{"comm"};
constexpr std::string_view JKEY_RUNQ_PID{"pid"};
constexpr std::string_view JKEY_RUNQ_PCOMM{"prev_comm"};
constexpr std::string_view JKEY_RUNQ_PPID{"prev_pid"};

// runqlat 标准桶：1us ~ 10s，覆盖 bpftrace runqlat.bt 的范围
const std::vector<double> runqlat_latency_buckets = {
    0.000001, 0.000002, 0.000005, 0.00001, 0.00002, 0.00005, // 1us ~ 50us
    0.0001,   0.0002,   0.0005,   0.001,   0.002,   0.005,   // 100us ~ 5ms
    0.01,     0.02,     0.05,     0.1,     0.2,     0.5,     // 10ms ~ 500ms
    1.0,      2.0,      5.0,      10.0,                      // 1s ~ 10s
};

} // namespace

namespace hebpf {
namespace services {
namespace runqlat {

Runqlat::Runqlat(std::unique_ptr<runqlat_bpf> skel)
    : ebpf::EbpfSkelIf<runqlat_bpf>{std::move(skel)} {
  updateClockOffset();
}

/**
 * @brief 获取 run queue 采集程序的名称
 *
 * @return std::string 程序名称
 */
std::string Runqlat::getName() const { return std::string{RUNQLAT_Sched_Switch}; }

/**
 * @brief 加载 run queue 采集程序
 *
 */
void Runqlat::load() {
  ebpf::EbpfSkelIf<runqlat_bpf>::load();

  auto fd = bpf_map__fd(skel_->maps.hebpf_rq_rb);
  if (fd < 0) {
    throw EXCEPT(fmt::format("Cannot get ringbuffer fd: {}", getName()), true);
  }
  ringbuff_ = std::make_unique<ebpf::Ringbuffer>(
      std::make_unique<Fd>(fd), EbpfSkelIf<runqlat_bpf>::io_ctx_,
      ebpf::Ringbuffer::RingbufferCb{
          FUNCTION_LINE, [this](void *data, size_t len) { return onEvent(data, len); }});
  ringbuff_->init();
}

/**
 * @brief 获取 run queue 采集程序 fd
 * @param hook_type 绑定类型
 *
 * @return int fd
 */
int Runqlat::getRunningFd(daemon::HookType hook_type) const {
  switch (hook_type) {
  case daemon::HookType::Sched_Switch:
    return bpf_program__fd(this->skel_->progs.hebpf_runqlat_sched_switch);
  case daemon::HookType::Sched_Wakeup:
    return bpf_program__fd(this->skel_->progs.hebpf_runqlat_sched_wakeup);
  default:
    return bpf_program__fd(this->skel_->progs.hebpf_runqlat_sched_wakeup_new);
  }
}

/**
 * @brief 获取 run queue 内核态采集程序收集的状态信息对接 Debug 服务器
 *
 * @return nlohmann::json JSON 对象
 */
nlohmann::json Runqlat::getDebugStatus() const {
  std::vector<rq_event> copy{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    copy.reserve(records_.size());
    copy.assign(records_.begin(), records_.end());
  }
  nlohmann::json result = nlohmann::json::array();
  for (const auto &record : copy) {
    result.emplace_back(nlohmann::json{{JKEY_RUNQ_PID, record.pid},
                                       {"tgid", record.tgid},
                                       {JKEY_RUNQ_PPID, record.prev_pid},
                                       {"prev_tgid", record.prev_tgid},
                                       {"wakeup_beijing", formatBeijingTime(record.wakeup_ts)},
                                       {"wakeup_mono", formatMonotonicSec(record.wakeup_ts)},
                                       {"switch_beijing", formatBeijingTime(record.switch_ts)},
                                       {"switch_mono", formatMonotonicSec(record.switch_ts)},
                                       {"latency", formatLatencyNs(record.latency_ns)},
                                       {JKEY_RUNQ_COMM, safeComm(record.comm)},
                                       {JKEY_RUNQ_PCOMM, safeComm(record.prev_comm)}});
  }
  return result;
}

/**
 * @brief 获取 run queue 内核态数据对接 Prometheus
 *
 * @return nlohmann::json 内核数据对象
 */
nlohmann::json Runqlat::getPrometheusStatus() {
  Metrics metrics{};
  std::vector<rq_event> records{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    records.reserve(records_.size());
    records.assign(records_.begin(), records_.end());
    records_.clear();
  }

  for (const auto &record : records) {
    const std::map<std::string, std::string> labels{
        {std::string{JKEY_RUNQ_PID}, std::to_string(record.pid)},
        {std::string{JKEY_RUNQ_COMM}, safeComm(record.comm)},
        {std::string{JKEY_RUNQ_PPID}, std::to_string(record.prev_pid)},
        {std::string{JKEY_RUNQ_PCOMM}, safeComm(record.prev_comm)},
    };

    // Histogram：延迟分布（单位：秒）
    KernelData latency_hist{};
    latency_hist.type_ = MetricType::HISTOGRAM;
    latency_hist.name_ = "hebpf_runqlat_latency_seconds";
    latency_hist.value_ = static_cast<double>(record.latency_ns) / 1e9;
    latency_hist.labels_ = labels;
    latency_hist.buckets_ = runqlat_latency_buckets;

    // Counter：事件总数
    KernelData events_total;
    events_total.type_ = MetricType::COUNTER;
    events_total.name_ = "hebpf_runqlat_events_total";
    events_total.value_ = 1.0;
    events_total.labels_ = labels;

    metrics.service_ = getName();
    metrics.metrics_.push_back(nlohmann::json(latency_hist));
    metrics.metrics_.push_back(nlohmann::json(events_total));
  }

  return metrics;
}

/**
 * @brief run queue 采集程序配置更新时触发
 *
 * @param config run queue 配置
 *
 * @return true 修改成功，false 修改失败
 */
bool Runqlat::onConfigUpdate(const nlohmann::json &config) {
  RunqlatConf conf{};
  try {
    conf = config;
  } catch (const std::exception &exc) {
    LOG(warn, "Runqlat configuration invalid: {}", exc.what());
    return false;
  }

  if (skel_ == nullptr || skel_->maps.hebpf_rq_config == nullptr) {
    LOG(error, "Runqlat skeleton not initialized");
    return false;
  }

  int map_fd = bpf_map__fd(skel_->maps.hebpf_rq_config);
  __u32 key = INDEX0_LATENCY_THRESHOLD_NS;
  __u64 val = conf.getLatency();
  if (bpf_map_update_elem(map_fd, &key, &val, BPF_ANY) != 0) {
    LOG(error, "Failed to update runqlat threshold: {}", ::strerror(errno));
    return false;
  }

  LOG(debug, "Runqlat latency threshold updated to {} ns", val);
  return true;
}

/**
 * @brief 格式化延迟事件
 *
 * @param ns 时间戳（单位纳秒）
 * @return std::string 字符串形式
 */
std::string Runqlat::formatLatencyNs(uint64_t ns) {
  char buf[32];
  if (ns >= 1'000'000'000ULL) {
    std::snprintf(buf, sizeof(buf), "%.2f s", static_cast<double>(ns) / 1e9);
  } else if (ns >= 1'000'000ULL) {
    std::snprintf(buf, sizeof(buf), "%.2f ms", static_cast<double>(ns) / 1e6);
  } else if (ns >= 1'000ULL) {
    std::snprintf(buf, sizeof(buf), "%.2f us", static_cast<double>(ns) / 1e3);
  } else {
    std::snprintf(buf, sizeof(buf), "%" PRIu64 " ns", ns);
  }
  return buf;
}

/**
 * @brief 安全读取 comm
 * @note 防止非 \0 结尾
 *
 * @return std::string_view 任务名称
 */
std::string Runqlat::safeComm(const char (&comm)[MAX_PROG_NAME]) {
  return std::string{comm, ::strnlen(comm, MAX_PROG_NAME)};
}

/**
 * @brief 内核事件到达
 *
 * @param data 内核数据
 * @param data_sz 内核数据长度
 * @return int 成功返回 0，失败返回 -1
 */
int Runqlat::onEvent([[maybe_unused]] void *data, [[maybe_unused]] size_t data_sz) {
  if (data_sz != sizeof(struct rq_event)) {
    GLOBAL_LOG(error, "{} eBPF cb: invalid event size: {}", getName(), data_sz);
    return -1;
  }

  auto *eve = static_cast<struct rq_event *>(data);

  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (records_.size() >= MAX_CAPACITY) {
      // TODO: 目前暂时 hard-code 规则数量为 MAX_CAPACITY
      records_.pop_front();
    }
    records_.push_back(*eve);
  }

  return 0;
}

/**
 * @brief 采样 CLOCK_REALTIME 与 CLOCK_MONOTONIC 的偏移
 *
 * @note 若系统发生大幅 NTP 跳变，可重新调用
 */
void Runqlat::updateClockOffset() {
  struct timespec ts_rt{};
  struct timespec ts_mono{};

  if (::clock_gettime(CLOCK_REALTIME, &ts_rt) != 0 ||
      ::clock_gettime(CLOCK_MONOTONIC, &ts_mono) != 0) {
    GLOBAL_LOG(error, "{}: clock_gettime failed: {}", getName(), ::strerror(errno));
    clock_offset_ns_ = 0;
    return;
  }

  const uint64_t rt_ns =
      static_cast<uint64_t>(ts_rt.tv_sec) * ns_per_sec + static_cast<uint64_t>(ts_rt.tv_nsec);
  const uint64_t mono_ns =
      static_cast<uint64_t>(ts_mono.tv_sec) * ns_per_sec + static_cast<uint64_t>(ts_mono.tv_nsec);

  clock_offset_ns_ = rt_ns - mono_ns;

  GLOBAL_LOG(info, "{}: clock offset (REALTIME - MONOTONIC) = {} ns", getName(), clock_offset_ns_);
}

/**
 * @brief 将 CLOCK_MONOTONIC 纳秒转换为北京时间字符串
 *
 * @param monotonic_ns bpf_ktime_get_ns() 返回的时间戳
 * @return std::string 形如 "2026-10-24 12:37:51.133"；转换失败返回 "invalid"
 */
std::string Runqlat::formatBeijingTime(uint64_t monotonic_ns) const {
  if (clock_offset_ns_ == 0) {
    return "uninitialized";
  }

  // MONOTONIC -> REALTIME(UTC)，再 +8h 得北京时间
  const uint64_t realtime_ns = monotonic_ns + clock_offset_ns_ + beijing_offset_ns;

  const time_t sec = static_cast<time_t>(realtime_ns / ns_per_sec);
  const uint64_t nsec = realtime_ns % ns_per_sec;

  struct tm tm_buf{};
  if (::gmtime_r(&sec, &tm_buf) == nullptr) {
    return "invalid";
  }

  char buf[64];
  std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%06lu", tm_buf.tm_year + 1900,
                tm_buf.tm_mon + 1, tm_buf.tm_mday, tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
                static_cast<unsigned long>(nsec / 1000ULL));
  return buf;
}

/**
 * @brief 把 CLOCK_MONOTONIC 纳秒格式化为"启动后秒数"，便于与 /proc/uptime 对照
 */
std::string Runqlat::formatMonotonicSec(uint64_t monotonic_ns) const {
  const uint64_t sec = monotonic_ns / ns_per_sec;
  const uint64_t nsec = monotonic_ns % ns_per_sec;
  char buf[48];
  std::snprintf(buf, sizeof(buf), "%" PRIu64 ".%09" PRIu64 "s", sec, nsec);
  return buf;
}

} // namespace runqlat
} // namespace services
} // namespace hebpf

/**
 * @brief 动态库导出接口：用来返回 eBPF 对象
 *
 * @return std::unique_ptr<hebpf::ebpf::EbpfIf> 对象
 */
extern "C" std::unique_ptr<hebpf::ebpf::EbpfIf> create_service() {
  return std::make_unique<hebpf::services::runqlat::Runqlat>();
}
