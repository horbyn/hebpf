// clang-format off
#include "service_map.h"
// clang-format on

namespace hebpf {
namespace ebpf {

void DlCloser::operator()(void *h) const noexcept {
  if (h) {
    dlclose(h);
  }
}

bool ServiceKey::operator==(const ServiceKey &other) const {
  return hook == other.hook && name == other.name;
}

size_t ServiceKeyHash::operator()(const ServiceKey &key) const noexcept {
  size_t h1 = std::hash<int>{}(static_cast<int>(key.hook.hook));
  size_t h2 = std::hash<int>{}(key.hook.hook_id);
  size_t h3 = std::hash<std::string>{}(key.name);
  return h1 ^ (h2 << 1) ^ (h3 << 2);
}

void to_json(nlohmann::json &json, const ServiceMap &map) {
  for (const auto &[key, ebpf] : map) {
    if (daemon::ConfigEbpf::isScheduler(key.name)) {
      continue;
    }
    json[key.name] = ebpf.instance->getDebugStatus();
  }
}

} // namespace ebpf
} // namespace hebpf
