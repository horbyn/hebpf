// clang-format off
#include "iotrace.h"
#include "src/common/common.h"
#include "src/data/kernel_data.h"
#include "src/data/metrics.h"
#include "src/data/queue.h"
#include "src/ebpf/ringbuffer.h"
#include "src/fd/fd.h"
#include "generated/ebpf_prog_map.h"
// clang-format on

namespace {

constexpr std::string_view JKEY_IOTRACE_DEV{"dev"};
constexpr std::string_view JKEY_IOTRACE_TYPE{"type"};
constexpr std::string_view JKEY_IOTRACE_COMM{"comm"};

// 将事件类型转换为可读字符串
std::string_view ioTypeToStr(unsigned int type) {
  switch (type) {
  case IO_TYPE_Q2C:
    return "Q2C"; // Queue to Complete (总延迟)
  case IO_TYPE_D2C:
    return "D2C"; // Dispatch to Complete (设备物理延迟)
  case IO_TYPE_I2D:
    return "I2D"; // Insert to Dispatch (调度器排队时间)
  default:
    return "unknown";
  }
}

// 安全读取 comm（防止非 \0 结尾）
std::string safeComm(const char (&comm)[16]) { return std::string{comm, strnlen(comm, 16)}; }

// 磁盘 I/O 延迟直方图桶：1us ~ 10s，覆盖绝大多数块设备场景
const std::vector<double> iotrace_latency_buckets = {
    0.000001, 0.000002, 0.000005, 0.00001, 0.00002, 0.00005, // 1us ~ 50us
    0.0001,   0.0002,   0.0005,   0.001,   0.002,   0.005,   // 100us ~ 5ms
    0.01,     0.02,     0.05,     0.1,     0.2,     0.5,     // 10ms ~ 500ms
    1.0,      2.0,      5.0,      10.0                       // 1s ~ 10s
};

} // namespace

namespace hebpf {
namespace services {
namespace iotrace {

// TODO: 暂定缓存大小
constexpr uint64_t MAX_CAPACITY{10240};

Iotrace::Iotrace(std::unique_ptr<iotrace_bpf> skel)
    : ebpf::EbpfSkelIf<iotrace_bpf>{std::move(skel)} {
  records_ = std::make_unique<Queue<hebpf_io_event>>(MAX_CAPACITY);
  records_->start();
}

Iotrace::~Iotrace() {
  if (records_ != nullptr) {
    records_->stop();
  }
}

/**
 * @brief 获取磁盘 io 采集程序的名称
 *
 * @return std::string 程序名称
 */
std::string Iotrace::getName() const { return std::string{IOTRACE_Block}; }

/**
 * @brief 加载磁盘 io 采集程序
 *
 */
void Iotrace::load() {
  switch (type_) {
  case daemon::HookType::Block_Bio_Queue:
    bpf_program__set_autoload(skel_->progs.hebpf_iotrace_block_bio_queue, true);
    break;
  case daemon::HookType::Block_Io_Start:
    bpf_program__set_autoload(skel_->progs.hebpf_iotrace_block_io_start, true);
    break;
  case daemon::HookType::Block_Rq_Complete:
    bpf_program__set_autoload(skel_->progs.hebpf_iotrace_block_rq_complete, true);
    break;
  default:
    bpf_program__set_autoload(skel_->progs.hebpf_iotrace_block_rq_issue, true);
    break;
  }

  ebpf::EbpfSkelIf<iotrace_bpf>::load();

  auto fd = bpf_map__fd(skel_->maps.hebpf_iotrc_rb);
  if (fd < 0) {
    throw EXCEPT(fmt::format("Cannot get ringbuffer fd: {}", getName()), true);
  }
  ringbuff_ = std::make_unique<ebpf::Ringbuffer>(
      std::make_unique<Fd>(fd), EbpfSkelIf<iotrace_bpf>::io_ctx_,
      ebpf::Ringbuffer::RingbufferCb{
          FUNCTION_LINE, [this](void *data, size_t len) { return onEvent(data, len); }});
  ringbuff_->init();
}

/**
 * @brief 获取磁盘 io 采集程序 fd
 * @param hook_type 绑定类型
 *
 * @return int fd
 */
int Iotrace::getRunningFd([[maybe_unused]] daemon::HookType hook_type) const {
  switch (type_) {
  case daemon::HookType::Block_Bio_Queue:
    return bpf_program__fd(skel_->progs.hebpf_iotrace_block_bio_queue);
  case daemon::HookType::Block_Io_Start:
    return bpf_program__fd(skel_->progs.hebpf_iotrace_block_io_start);
  case daemon::HookType::Block_Rq_Complete:
    return bpf_program__fd(skel_->progs.hebpf_iotrace_block_rq_complete);
  default:
    return bpf_program__fd(skel_->progs.hebpf_iotrace_block_rq_issue);
  }
}

/**
 * @brief 获取磁盘 io 内核态数据对接 Prometheus
 *
 * @return nlohmann::json 内核数据对象
 */
nlohmann::json Iotrace::getPrometheusStatus() {
  Metrics metrics{};
  metrics.service_ = getName();

  auto records = records_->drain();

  for (const auto &record : records) {
    const KernelData::KernelDataMap labels{
        {std::string{JKEY_IOTRACE_DEV}, std::to_string(record.dev)},
        {std::string{JKEY_IOTRACE_TYPE}, std::string(ioTypeToStr(record.type))},
        {std::string{JKEY_IOTRACE_COMM}, safeComm(record.comm)},
    };

    // Histogram：延迟分布（单位：秒）
    KernelData latency_hist{MetricType::HISTOGRAM, "hebpf_iotrace_latency_seconds",
                            static_cast<double>(record.latency_ns) / 1e9, labels,
                            iotrace_latency_buckets};
    metrics.metrics_.push_back(latency_hist);

    // Counter：事件总数
    KernelData events_total{MetricType::COUNTER, "hebpf_iotrace_events_total", 1.0, labels};
    metrics.metrics_.push_back(events_total);

    // Counter：传输的字节数（用于计算吞吐量）
    KernelData bytes_total{MetricType::COUNTER, "hebpf_iotrace_bytes_total",
                           static_cast<double>(record.bytes), labels};
    metrics.metrics_.push_back(bytes_total);
  } // end for()

  return metrics;
}

/**
 * @brief 磁盘 io 采集程序配置更新时触发
 *
 * @param config 磁盘 io 配置
 *
 * @return true 修改成功，false 修改失败
 */
bool Iotrace::onConfigUpdate([[maybe_unused]] const nlohmann::json &config) { return false; }

int Iotrace::onEvent(void *data, size_t data_sz) {
  if (data_sz != sizeof(struct hebpf_io_event)) {
    GLOBAL_LOG(error, "{} eBPF cb: invalid event size: {}", getName(), data_sz);
    return -1;
  }

  auto *eve = static_cast<struct hebpf_io_event *>(data);
  records_->push(*eve);

  return 0;
}

} // namespace iotrace
} // namespace services
} // namespace hebpf

/**
 * @brief 动态库导出接口：用来返回 eBPF 对象
 *
 * @return std::unique_ptr<hebpf::ebpf::EbpfIf> 对象
 */
extern "C" std::unique_ptr<hebpf::ebpf::EbpfIf> create_service() {
  return std::make_unique<hebpf::services::iotrace::Iotrace>();
}
