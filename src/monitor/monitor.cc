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
                       .Help("auto-generated from eBPF service")
                       .Register(*registry_);
    auto &counter = family.Add(labels);
    counters_.emplace(key, std::ref(counter));
    counter.Increment(value_inc);
  } else {
    it->second.get().Increment(value_inc);
  }
}

/**
 * @brief 根据哈希表转换为一个 key
 * @note 这个 key 的来源是一个哈希表的数据，如 abc{a=1,b=2}，
 * 这里形参用 map 就是为了让 key 总是有序，即总是 abc{a=1,b=2}，
 * 而不会有时候变成 abc{b=2,a=1}，从而保证同一个哈希表转换的 key 总是唯一的
 *
 * @param name
 * @param labels
 * @return std::string
 */
std::string Monitor::makeKey(std::string_view name,
                             const std::map<std::string, std::string> &labels) const {
  // TODO: 如果 labes 数据量很大, map 可能会很耗时
  std::ostringstream oss{};
  oss << name;
  for (const auto &[k, v] : labels) {
    oss << "|" << k << "=" << v;
  }
  return oss.str();
}

} // namespace monitor
} // namespace hebpf
