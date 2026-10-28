#pragma once

// clang-format off
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {

constexpr std::string_view JKEY_USERMODE_TYPE{"type"};
constexpr std::string_view JKEY_USERMODE_NAME{"name"};
constexpr std::string_view JKEY_USERMODE_VALUE{"value"};
constexpr std::string_view JKEY_USERMODE_LABEL{"labels"};
constexpr std::string_view JKEY_USERMODE_BUCKETS{"buckets"};

/**
 * @brief 指标类型
 */
enum class MetricType : std::uint8_t {
  COUNTER = 0,   // 单值累加
  GAUGE = 1,     // 单值瞬时
  HISTOGRAM = 2, // 值分布
};

// 让 nlohmann::json 自动序列化/反序列化 MetricType（字符串形式，便于调试）
NLOHMANN_JSON_SERIALIZE_ENUM(MetricType, {
                                             {MetricType::COUNTER, "counter"},
                                             {MetricType::GAUGE, "gauge"},
                                             {MetricType::HISTOGRAM, "histogram"},
                                         })

/**
 * @brief 从内核态采集的、准备上报到 Prometheus 的一条指标
 */
class KernelData final {
public:
  friend void from_json(const nlohmann::json &json, KernelData &event);
  friend void to_json(nlohmann::json &json, const KernelData &event);

  MetricType type_{MetricType::COUNTER};
  std::string name_;
  double value_{0.0};
  std::map<std::string, std::string> labels_;
  std::vector<double> buckets_; // 仅 HISTOGRAM 使用
};

} // namespace hebpf
