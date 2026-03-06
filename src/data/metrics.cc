// clang-format off
#include "metrics.h"
// clang-format on

namespace hebpf {

void from_json(const nlohmann::json &json, Metrics &metrics) {
  if (json.contains(JKEY_METRICS_SERVICE)) {
    metrics.service_ = json.at(JKEY_METRICS_SERVICE).get<std::string>();
  }
  if (json.contains(JKEY_METRICS_METRICS)) {
    metrics.metrics_ = json.at(JKEY_METRICS_METRICS).get<std::vector<nlohmann::json>>();
  }
}

void to_json(nlohmann::json &json, const Metrics &metrics) {
  json = nlohmann::json{{JKEY_METRICS_SERVICE, metrics.service_},
                        {JKEY_METRICS_METRICS, metrics.metrics_}};
}

} // namespace hebpf
