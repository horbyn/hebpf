#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <vector>
#include "yaml-cpp/yaml.h"
#include "src/common/enum_name.hpp"
#include "src/log/logger.h"
#include "hebpf_version.h"
// clang-format on

namespace hebpf {
namespace daemon {

constexpr std::string_view CONFIGS_DEFAULT{HEBPF_PROJECT ".yaml"};
constexpr std::string_view CONFIGS_PROMETHEUS{"prometheus"};
constexpr std::string_view CONFIGS_PROM_ENABLED{"enabled"};
constexpr std::string_view CONFIGS_PROM_LISTEN{"listen"};
constexpr std::string_view CONFIGS_EBPFSO{"ebpf"};

constexpr std::string_view DEFAULT_PROM_LISTEN{"0.0.0.0:8080"};

class Configs final : public log::Loggable<log::Id::daemon> {
public:
  Configs() = default;
  Configs(std::string_view configs_path);

  static Configs loadFromConfig(std::string_view filepath);

  void setPrometheusEnabled(bool enabled);
  bool getPrometheusEnabled() const;

  void setPrometheusListen(std::string_view listen);
  std::string getPrometheusListen() const;

  void setEbpf(const std::vector<std::string> &ebpf_so);
  std::vector<std::string> getEbpf() const;

  bool operator==(const Configs &other) const;
  bool operator!=(const Configs &other) const;

private:
  bool prometheus_enabled_{false};
  std::string prometheus_listen_{DEFAULT_PROM_LISTEN};
  std::vector<std::string> ebpf_;
};

} // namespace daemon
} // namespace hebpf

namespace YAML {

template <>
struct convert<hebpf::daemon::Configs> {
  static Node encode(const hebpf::daemon::Configs &conf) {
    Node node{};
    Node prometheus_node{};
    prometheus_node[hebpf::daemon::CONFIGS_PROM_ENABLED] = conf.getPrometheusEnabled();
    prometheus_node[hebpf::daemon::CONFIGS_PROM_LISTEN] = conf.getPrometheusListen();
    node[hebpf::daemon::CONFIGS_PROMETHEUS] = prometheus_node;
    auto vec = conf.getEbpf();
    if (!vec.empty()) {
      node[hebpf::daemon::CONFIGS_EBPFSO] = conf.getEbpf();
    }
    return node;
  }

  static bool decode(const Node &node, hebpf::daemon::Configs &conf) {
    if (node[hebpf::daemon::CONFIGS_PROMETHEUS]) {
      auto prom_node = node[hebpf::daemon::CONFIGS_PROMETHEUS];
      if (prom_node[hebpf::daemon::CONFIGS_PROM_ENABLED]) {
        conf.setPrometheusEnabled(prom_node[hebpf::daemon::CONFIGS_PROM_ENABLED].as<bool>());
      }
      if (prom_node[hebpf::daemon::CONFIGS_PROM_LISTEN]) {
        conf.setPrometheusListen(prom_node[hebpf::daemon::CONFIGS_PROM_LISTEN].as<std::string>());
      }
    }
    if (node[hebpf::daemon::CONFIGS_EBPFSO]) {
      conf.setEbpf(node[hebpf::daemon::CONFIGS_EBPFSO].as<std::vector<std::string>>());
    }
    return true;
  }
};

} // namespace YAML
