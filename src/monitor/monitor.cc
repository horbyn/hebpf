// clang-format off
#include "monitor.h"
#include <sstream>
#include "src/common/assert.h"
#include "src/common/exception.h"
// clang-format on

namespace hebpf {
namespace monitor {

Monitor::Monitor(std::string_view bind_address)
    : exposer_{std::make_unique<prometheus::Exposer>(std::string{bind_address})},
      registry_{std::make_shared<prometheus::Registry>()} {
  exposer_->RegisterCollectable(registry_);
}

/**
 * @brief 递增指标的计数器
 *
 * @param name 指标名称
 * @param labels 指标的标签
 * @param value 数值（默认每次增加 1）
 */
void Monitor::incrementCounter(std::string_view name,
                               const std::map<std::string, std::string> &labels, double value) {
  ASSERT(registry_ != nullptr);

  std::lock_guard<std::mutex> lock{mutex_};
  auto key = makeKey(name, labels);
  if (key.empty()) {
    throw EXCEPT("invalid key");
  }

  auto value_inc = static_cast<uint64_t>(value);
  auto it = counters_.find(key);
  if (it == counters_.end()) {
    auto &family = prometheus::BuildCounter()
                       .Name(std::string{name})
                       .Help("eBPF counter metric")
                       .Register(*registry_);
    auto &counter = family.Add(labels);
    counters_.emplace(key, std::ref(counter));
    counter.Increment(value_inc);
  } else {
    it->second.get().Increment(value_inc);
  }
}

/**
 * @brief 记录一次 Histogram 观测
 *
 * @param name   指标名
 * @param labels 标签
 * @param value  本次观测值（与桶单位一致）
 * @param buckets 桶边界（升序）；同名同标签的桶必须一致
 */
void Monitor::observeHistogram(std::string_view name,
                               const std::map<std::string, std::string> &labels, double value,
                               const std::vector<double> &buckets) {
  ASSERT(registry_ != nullptr);
  if (buckets.empty()) {
    throw EXCEPT("histogram buckets must not be empty");
  }

  std::lock_guard<std::mutex> lock{mutex_};
  auto key = makeKey(name, labels);
  if (key.empty()) {
    throw EXCEPT("invalid key");
  }

  auto it = histograms_.find(key);
  if (it == histograms_.end()) {
    auto &family = prometheus::BuildHistogram()
                       .Name(std::string{name})
                       .Help("eBPF histogram metric")
                       .Register(*registry_);
    // 同名同标签首次创建时确定桶边界；后续调用忽略 buckets 参数
    auto &histogram = family.Add(labels, buckets);
    histograms_.emplace(key, std::ref(histogram));
    histogram.Observe(value);
  } else {
    it->second.get().Observe(value);
  }
}

/**
 * @brief 设置 Gauge 当前值
 *
 * @param name 指标名
 * @param labels 标签
 * @param value 本次观测值
 */
void Monitor::setGauge(std::string_view name, const std::map<std::string, std::string> &labels,
                       double value) {
  ASSERT(registry_ != nullptr);

  std::lock_guard<std::mutex> lock{mutex_};
  auto key = makeKey(name, labels);
  if (key.empty()) {
    throw EXCEPT("invalid key");
  }

  auto it = gauges_.find(key);
  if (it == gauges_.end()) {
    auto &family = prometheus::BuildGauge()
                       .Name(std::string{name})
                       .Help("eBPF gauge metric")
                       .Register(*registry_);
    auto &gauge = family.Add(labels);
    gauges_.emplace(key, std::ref(gauge));
    gauge.Set(value);
  } else {
    it->second.get().Set(value);
  }
}

std::string Monitor::makeKey(std::string_view name,
                             const std::map<std::string, std::string> &labels) const {
  // std::map 天然有序，保证同名同标签的 key 始终唯一
  std::ostringstream oss{};
  oss << name;
  for (const auto &[k, v] : labels) {
    oss << "|" << k << "=" << v;
  }
  return oss.str();
}

} // namespace monitor
} // namespace hebpf
