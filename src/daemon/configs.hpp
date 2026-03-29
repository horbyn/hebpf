#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <unordered_map>
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
constexpr std::string_view CONFIGS_EBPF_LIB{"lib"};
constexpr std::string_view CONFIGS_EBPF_CONFIG{"config"};

constexpr std::string_view DEFAULT_PROM_LISTEN{"0.0.0.0:8080"};

class ConfigEbpf final : public log::Loggable<log::Id::daemon> {
public:
  ConfigEbpf() = default;
  ConfigEbpf(std::string_view lib, std::string_view config);

  void setLib(std::string_view lib);
  std::string getLib() const;

  void setConfig(std::string_view config);
  std::string getConfig() const;

  bool operator==(const ConfigEbpf &other) const;
  bool operator!=(const ConfigEbpf &other) const;

private:
  std::string lib_;
  std::string config_;
};

class Configs final : public log::Loggable<log::Id::daemon> {
public:
  using EbpfMap = std::map<std::string, ConfigEbpf>;

  Configs() = default;
  Configs(std::string_view configs_path);

  static Configs loadFromConfig(std::string_view filepath);

  void setPrometheusEnabled(bool enabled);
  bool getPrometheusEnabled() const;

  void setPrometheusListen(std::string_view listen);
  std::string getPrometheusListen() const;

  void setEbpfs(const EbpfMap &ebpf_so);
  EbpfMap getEbpfs() const;

  void appendEbpf(std::string_view name, std::string_view lib, std::string_view config);
  void deleteEbpf(std::string_view name);

  bool operator==(const Configs &other) const;
  bool operator!=(const Configs &other) const;

private:
  bool prometheus_enabled_{false};
  std::string prometheus_listen_{DEFAULT_PROM_LISTEN};
  EbpfMap ebpfs_;
};

} // namespace daemon
} // namespace hebpf

namespace YAML {

template <>
struct convert<hebpf::daemon::ConfigEbpf> {
  static Node encode(const hebpf::daemon::ConfigEbpf &conf) {
    Node node{};
    node[hebpf::daemon::CONFIGS_EBPF_LIB] = conf.getLib();
    node[hebpf::daemon::CONFIGS_EBPF_CONFIG] = conf.getConfig();
    return node;
  }

  static bool decode(const Node &node, hebpf::daemon::ConfigEbpf &conf) {
    if (node[hebpf::daemon::CONFIGS_EBPF_LIB]) {
      conf.setLib(node[hebpf::daemon::CONFIGS_EBPF_LIB].as<std::string>());
    }
    if (node[hebpf::daemon::CONFIGS_EBPF_CONFIG]) {
      conf.setConfig(node[hebpf::daemon::CONFIGS_EBPF_CONFIG].as<std::string>());
    }
    return true;
  }
};

template <>
struct convert<hebpf::daemon::Configs> {
  static Node encode(const hebpf::daemon::Configs &conf) {
    Node node{};
    Node prometheus_node{};
    prometheus_node[hebpf::daemon::CONFIGS_PROM_ENABLED] = conf.getPrometheusEnabled();
    prometheus_node[hebpf::daemon::CONFIGS_PROM_LISTEN] = conf.getPrometheusListen();
    node[hebpf::daemon::CONFIGS_PROMETHEUS] = prometheus_node;
    auto vector = conf.getEbpfs();
    if (!vector.empty()) {
      node[hebpf::daemon::CONFIGS_EBPFSO] = vector;
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
      conf.setEbpfs(node[hebpf::daemon::CONFIGS_EBPFSO].as<hebpf::daemon::Configs::EbpfMap>());
    }
    return true;
  }
};

} // namespace YAML
