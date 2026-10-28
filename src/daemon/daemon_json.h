#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {
namespace daemon {

constexpr std::string_view JKEY_DAEMONJSON_RUN{"running"};
constexpr std::string_view JKEY_DAEMONJSON_QEMPTY{"queue_empty"};
constexpr std::string_view JKEY_DAEMONJSON_QFULL{"queue_full"};
constexpr std::string_view JKEY_DAEMONJSON_CONF{"configurations"};
constexpr std::string_view JKEY_DAEMONJSON_REGION{"regions"};
constexpr std::string_view JKEY_DAEMONJSON_SERV{"services"};
constexpr std::string_view JKEY_DAEMONJSON_PINNED{"pinned_maps"};

class DaemonJson final {
public:
  explicit DaemonJson() = default;
  explicit DaemonJson(bool running, bool queue_empty, bool queue_full, const nlohmann::json &conf,
                      const nlohmann::json &regions, const nlohmann::json &services,
                      const nlohmann::json &pinned_maps);

  void setRunning(bool running);
  bool getRunning() const;

  void setQueueEmpty(bool queue_empty);
  bool getQueueEmpty() const;

  void setQueueFull(bool queue_full);
  bool getQueueFull() const;

  void setConfigurations(const nlohmann::json &conf);
  nlohmann::json getConfigurations() const;

  void setRegions(const nlohmann::json &region);
  nlohmann::json getRegions() const;

  void setServices(const nlohmann::json &services);
  nlohmann::json getServices() const;

  void setPinnedMaps(const nlohmann::json &pinned);
  nlohmann::json getPinnedMaps() const;

  friend void to_json(nlohmann::json &json, const DaemonJson &daemon);

private:
  bool running_;
  bool queue_empty_;
  bool queue_full_;
  nlohmann::json configurations_;
  nlohmann::json regions_;
  nlohmann::json services_;
  nlohmann::json pinned_maps_;
};

} // namespace daemon
} // namespace hebpf
