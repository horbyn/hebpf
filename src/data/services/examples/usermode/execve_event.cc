// clang-format off
#include "execve_event.h"
// clang-format on

namespace hebpf {
namespace services {
namespace examples {

void from_json(const nlohmann::json &json, ExecveEvent &event) {
  if (json.contains(JKEY_USERMODE_NAME)) {
    event.name_ = json.at(JKEY_USERMODE_NAME).get<std::string>();
  }
  if (json.contains(JKEY_USERMODE_VALUE)) {
    event.value_ = json.at(JKEY_USERMODE_VALUE).get<double>();
  }
  if (json.contains(JKEY_USERMODE_LABEL)) {
    event.labels_ = json.at(JKEY_USERMODE_LABEL).get<std::map<std::string, std::string>>();
  }
}

void to_json(nlohmann::json &json, const ExecveEvent &event) {
  json = nlohmann::json{{JKEY_USERMODE_NAME, event.name_},
                        {JKEY_USERMODE_VALUE, event.value_},
                        {JKEY_USERMODE_LABEL, event.labels_}};
}

} // namespace examples
} // namespace services
} // namespace hebpf
