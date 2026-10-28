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
    event.labels_ = json.at(JKEY_USERMODE_LABEL).get<std::map<std::string, std::string>>();
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

} // namespace hebpf
