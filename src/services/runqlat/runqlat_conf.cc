// clang-format off
#include "runqlat_conf.h"
// clang-format on

namespace hebpf {
namespace services {
namespace runqlat {

/**
 * @brief 设置延迟门槛（单位纳秒）
 *
 * @param latency 延迟
 */
void RunqlatConf::setLatency(uint64_t latency) { latency_threshold_ns_ = latency; }

/**
 * @brief 获取延迟门槛
 *
 * @return uint64_t 延迟门槛（纳秒）
 */
uint64_t RunqlatConf::getLatency() const noexcept { return latency_threshold_ns_; }

void from_json(const nlohmann::json &json, RunqlatConf &conf) {
  if (json.contains(JKEY_RUNQCONF_LATENCY)) {
    conf.latency_threshold_ns_ = json.at(JKEY_RUNQCONF_LATENCY).get<uint64_t>();
  }
}

void to_json(nlohmann::json &json, const RunqlatConf &conf) {
  json = nlohmann::json{{JKEY_RUNQCONF_LATENCY, conf.latency_threshold_ns_}};
}

} // namespace runqlat
} // namespace services
} // namespace hebpf
