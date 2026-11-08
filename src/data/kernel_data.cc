// clang-format off
#include "kernel_data.h"
// clang-format on

namespace hebpf {

void from_json(const nlohmann::json &json, KernelData &event) {
  if (json.contains(JKEY_USERMODE_TYPE)) {
    event.type_ = json.at(JKEY_USERMODE_TYPE).get<MetricType>();
  }
  if (json.contains(JKEY_USERMODE_NAME)) {
    event.name_ = json.at(JKEY_USERMODE_NAME).get<std::string>();
  }
  if (json.contains(JKEY_USERMODE_VALUE)) {
    event.value_ = json.at(JKEY_USERMODE_VALUE).get<double>();
  }
  if (json.contains(JKEY_USERMODE_LABEL)) {
    event.labels_ = json.at(JKEY_USERMODE_LABEL).get<KernelData::KernelDataMap>();
  }
  if (json.contains(JKEY_USERMODE_BUCKETS)) {
    event.buckets_ = json.at(JKEY_USERMODE_BUCKETS).get<std::vector<double>>();
  }
}

void to_json(nlohmann::json &json, const KernelData &event) {
  json = nlohmann::json{
      {JKEY_USERMODE_TYPE, event.type_},
      {JKEY_USERMODE_NAME, event.name_},
      {JKEY_USERMODE_VALUE, event.value_},
      {JKEY_USERMODE_LABEL, event.labels_},
  };
  if (!event.buckets_.empty()) {
    json[JKEY_USERMODE_BUCKETS] = event.buckets_;
  }
}

KernelData::KernelData(MetricType type, const std::string &name, double value,
                       const KernelData::KernelDataMap &labels, const std::vector<double> &buckets)
    : type_{type}, name_{name}, value_{value}, labels_{labels}, buckets_{buckets} {}

/**
 * @brief 获取指标类型
 *
 * @return MetricType 指标类型
 */
MetricType KernelData::getType() const noexcept { return type_; }

/**
 * @brief 获取内核态数据名称
 *
 * @return std::string 名称
 */
std::string KernelData::getName() const { return name_; }

/**
 * @brief 获取内核态数据取值
 *
 * @return double 内核态数据
 */
double KernelData::getValue() const noexcept { return value_; }

/**
 * @brief 获取指标
 *
 * @return KernelData::KernelDataMap 指标
 */
KernelData::KernelDataMap KernelData::getLabels() const { return labels_; }

/**
 * @brief 获取直方图横坐标
 *
 * @return std::vector<double> 指标数组
 */
std::vector<double> KernelData::getBuckets() const { return buckets_; }

} // namespace hebpf
