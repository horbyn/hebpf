#pragma once

// clang-format off
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "yaml-cpp/yaml.h"
#include "spdlog/fmt/ranges.h"
#include "src/common/enum_name.hpp"
#include "src/log/logger.h"
#include "hebpf_version.h"
// clang-format on

namespace hebpf {
namespace daemon {

constexpr std::string_view CONFIGS_DEFAULT{HEBPF_PROJECT ".yaml"};

constexpr std::string_view CONFIGS_PROMETHEUS{"prometheus"};
constexpr std::string_view CONFIGS_ENABLED{"enabled"};
constexpr std::string_view CONFIGS_PROM_LISTEN{"listen"};

constexpr std::string_view CONFIGS_DEBUG_SERVER{"debug_server"};
constexpr std::string_view CONFIGS_DBGSERV_ADDR{"address"};
constexpr std::string_view CONFIGS_DBGSERV_PORT{"port"};

constexpr std::string_view CONFIGS_LOKI{"loki"};
constexpr std::string_view CONFIGS_LOKI_ENABLED{"enabled"};
constexpr std::string_view CONFIGS_LOKI_HOST{"host"};
constexpr std::string_view CONFIGS_LOKI_PORT{"port"};
constexpr std::string_view CONFIGS_LOKI_PATH{"path"};
constexpr std::string_view CONFIGS_LOKI_BATCH_SIZE{"batch_size"};
constexpr std::string_view CONFIGS_LOKI_FLUSH_INTERVAL{"flush_interval"};

constexpr std::string_view CONFIGS_EBPFSO{"ebpf"};
constexpr std::string_view CONFIGS_EBPF_LIB{"lib"};
constexpr std::string_view CONFIGS_EBPF_CONFIG{"config"};
constexpr std::string_view CONFIGS_EBPF_HOOK{"hook"};
constexpr std::string_view CONFIGS_EBPF_IFINDEX{"ifindex"};
constexpr std::string_view CONFIGS_EBPF_ORDER{"order"};

constexpr std::string_view DEFAULT_PROM_LISTEN{"0.0.0.0:8080"};
constexpr std::string_view DEFAULT_DBGSERV_ADDR{"127.0.0.1"};
constexpr uint16_t DEFAULT_DBGSERV_PORT{9999};
constexpr std::string_view DEFAULT_LOKI_HOST{"localhost"};
constexpr uint16_t DEFAULT_LOKI_PORT{3100};
constexpr std::string_view DEFAULT_LOKI_PATH{"loki/api/v1/push"};

enum class HookType : uint8_t { TC, XDP_GENERIC, XDP_NATIVE, XDP_OFFLOAD, KProbe, UNKNOWN };

class ConfigEbpf final : public log::Loggable<log::Id::daemon> {
public:
  ConfigEbpf();
  ConfigEbpf(std::string_view lib, std::string_view config, HookType hook, int ifindex, int order);

  void setLib(std::string_view lib);
  std::string getLib() const;

  void setConfig(std::string_view config);
  std::string getConfig() const;

  void setHook(HookType hook);
  HookType getHook() const noexcept;

  void setIfindex(int ifindex);
  int getIfindex() const noexcept;

  void setOrder(int order);
  int getOrder() const noexcept;

  bool operator==(const ConfigEbpf &other) const;
  bool operator!=(const ConfigEbpf &other) const;

private:
  std::string lib_;
  std::string config_;
  HookType hook_;
  int ifindex_;
  int order_;
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

  void setDebugServerEnabled(bool enabled);
  bool getDebugServerEnabled() const;

  void setDebugServerAddr(std::string_view address);
  std::string getDebugServerAddr() const;

  void setDebugServerPort(uint16_t port);
  uint16_t getDebugServerPort() const;

  void setLokiEnabled(bool enabled);
  bool getLokiEnabled() const;

  void setLokiHost(std::string_view host);
  std::string getLokiHost() const;

  void setLokiPort(uint16_t port);
  uint16_t getLokiPort() const;

  void setLokiPath(std::string_view path);
  std::string getLokiPath() const;

  void setLokiBatchSize(int size);
  int getLokiBatchSize() const;

  void setLokiFlushInterval(int seconds);
  int getLokiFlushInterval() const;

  void setEbpfs(const EbpfMap &ebpf_so);
  EbpfMap getEbpfs() const;

  void appendEbpf(std::string_view name, std::string_view lib, std::string_view config,
                  HookType hook, int ifindex, int order);
  void deleteEbpf(std::string_view name);

  bool operator==(const Configs &other) const;
  bool operator!=(const Configs &other) const;

private:
  bool prometheus_enabled_{false};
  std::string prometheus_listen_{std::string{DEFAULT_PROM_LISTEN}};
  bool dbgserv_enabled_{false};
  std::string dbgserv_addr_{std::string{DEFAULT_DBGSERV_ADDR}};
  uint16_t dbgserv_port_{DEFAULT_DBGSERV_PORT};
  bool loki_enabled_{false};
  std::string loki_host_{std::string{DEFAULT_LOKI_HOST}};
  uint16_t loki_port_{DEFAULT_LOKI_PORT};
  std::string loki_path_{std::string{DEFAULT_LOKI_PATH}};
  int loki_batch_size_{100};
  int loki_flush_interval_{5};
  EbpfMap ebpfs_{{"example", ConfigEbpf{}}};
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
    node[hebpf::daemon::CONFIGS_EBPF_HOOK] = std::string{hebpf::enumName(conf.getHook())};
    node[hebpf::daemon::CONFIGS_EBPF_IFINDEX] = conf.getIfindex();
    node[hebpf::daemon::CONFIGS_EBPF_ORDER] = conf.getOrder();
    return node;
  }

  static bool decode(const Node &node, hebpf::daemon::ConfigEbpf &conf) {
    if (node[hebpf::daemon::CONFIGS_EBPF_LIB]) {
      conf.setLib(node[hebpf::daemon::CONFIGS_EBPF_LIB].as<std::string>());
    }
    if (node[hebpf::daemon::CONFIGS_EBPF_CONFIG]) {
      conf.setConfig(node[hebpf::daemon::CONFIGS_EBPF_CONFIG].as<std::string>());
    }
    if (node[hebpf::daemon::CONFIGS_EBPF_HOOK]) {
      auto hook_opt = hebpf::stringEnum<hebpf::daemon::HookType>(
          node[hebpf::daemon::CONFIGS_EBPF_HOOK].as<std::string>());
      if (hook_opt) {
        conf.setHook(*hook_opt);
      } else {
        auto vec = hebpf::enumNameList<hebpf::daemon::HookType>();
        vec.pop_back(); // UNKNOWN
        std::string desc = fmt::format("{}", fmt::join(vec, "/"));
        // TODO: 整合到日志系统
        fprintf(stderr, "Configuration error: unknown hook \"%s\", expected %s\n",
                node[hebpf::daemon::CONFIGS_EBPF_HOOK].as<std::string>().c_str(), desc.c_str());
        conf.setHook(hebpf::daemon::HookType::UNKNOWN);
      }
    }
    if (node[hebpf::daemon::CONFIGS_EBPF_IFINDEX]) {
      conf.setIfindex(node[hebpf::daemon::CONFIGS_EBPF_IFINDEX].as<int>());
    }
    if (node[hebpf::daemon::CONFIGS_EBPF_ORDER]) {
      conf.setOrder(node[hebpf::daemon::CONFIGS_EBPF_ORDER].as<int>());
    }
    return true;
  }
};

template <>
struct convert<hebpf::daemon::Configs> {
  static Node encode(const hebpf::daemon::Configs &conf) {
    Node node{};
    Node prometheus_node{};
    prometheus_node[hebpf::daemon::CONFIGS_ENABLED] = conf.getPrometheusEnabled();
    prometheus_node[hebpf::daemon::CONFIGS_PROM_LISTEN] = conf.getPrometheusListen();
    node[hebpf::daemon::CONFIGS_PROMETHEUS] = prometheus_node;

    Node debug_node{};
    debug_node[hebpf::daemon::CONFIGS_ENABLED] = conf.getDebugServerEnabled();
    debug_node[hebpf::daemon::CONFIGS_DBGSERV_ADDR] = conf.getDebugServerAddr();
    debug_node[hebpf::daemon::CONFIGS_DBGSERV_PORT] = conf.getDebugServerPort();
    node[hebpf::daemon::CONFIGS_DEBUG_SERVER] = debug_node;

    Node loki_node{};
    loki_node[hebpf::daemon::CONFIGS_ENABLED] = conf.getLokiEnabled();
    loki_node[hebpf::daemon::CONFIGS_LOKI_HOST] = conf.getLokiHost();
    loki_node[hebpf::daemon::CONFIGS_LOKI_PORT] = conf.getLokiPort();
    loki_node[hebpf::daemon::CONFIGS_LOKI_PATH] = conf.getLokiPath();
    loki_node[hebpf::daemon::CONFIGS_LOKI_BATCH_SIZE] = conf.getLokiBatchSize();
    loki_node[hebpf::daemon::CONFIGS_LOKI_FLUSH_INTERVAL] = conf.getLokiFlushInterval();
    node[hebpf::daemon::CONFIGS_LOKI] = loki_node;

    auto vector = conf.getEbpfs();
    if (!vector.empty()) {
      node[hebpf::daemon::CONFIGS_EBPFSO] = vector;
    }
    return node;
  }

  static bool decode(const Node &node, hebpf::daemon::Configs &conf) {
    if (node[hebpf::daemon::CONFIGS_PROMETHEUS]) {
      auto prom_node = node[hebpf::daemon::CONFIGS_PROMETHEUS];
      if (prom_node[hebpf::daemon::CONFIGS_ENABLED]) {
        conf.setPrometheusEnabled(prom_node[hebpf::daemon::CONFIGS_ENABLED].as<bool>());
      }
      if (prom_node[hebpf::daemon::CONFIGS_PROM_LISTEN]) {
        conf.setPrometheusListen(prom_node[hebpf::daemon::CONFIGS_PROM_LISTEN].as<std::string>());
      }
    }
    if (node[hebpf::daemon::CONFIGS_DEBUG_SERVER]) {
      auto debug_node = node[hebpf::daemon::CONFIGS_DEBUG_SERVER];
      if (debug_node[hebpf::daemon::CONFIGS_ENABLED]) {
        conf.setDebugServerEnabled(debug_node[hebpf::daemon::CONFIGS_ENABLED].as<bool>());
      }
      if (debug_node[hebpf::daemon::CONFIGS_DBGSERV_ADDR]) {
        conf.setDebugServerAddr(debug_node[hebpf::daemon::CONFIGS_DBGSERV_ADDR].as<std::string>());
      }
      if (debug_node[hebpf::daemon::CONFIGS_DBGSERV_PORT]) {
        conf.setDebugServerPort(debug_node[hebpf::daemon::CONFIGS_DBGSERV_PORT].as<uint16_t>());
      }
    }
    if (node[hebpf::daemon::CONFIGS_LOKI]) {
      auto loki_node = node[hebpf::daemon::CONFIGS_LOKI];
      if (loki_node[hebpf::daemon::CONFIGS_LOKI_ENABLED]) {
        conf.setLokiEnabled(loki_node[hebpf::daemon::CONFIGS_LOKI_ENABLED].as<bool>());
      }
      if (loki_node[hebpf::daemon::CONFIGS_LOKI_HOST]) {
        conf.setLokiHost(loki_node[hebpf::daemon::CONFIGS_LOKI_HOST].as<std::string>());
      }
      if (loki_node[hebpf::daemon::CONFIGS_LOKI_PORT]) {
        conf.setLokiPort(loki_node[hebpf::daemon::CONFIGS_LOKI_PORT].as<uint16_t>());
      }
      if (loki_node[hebpf::daemon::CONFIGS_LOKI_PATH]) {
        conf.setLokiPath(loki_node[hebpf::daemon::CONFIGS_LOKI_PATH].as<std::string>());
      }
      if (loki_node[hebpf::daemon::CONFIGS_LOKI_BATCH_SIZE]) {
        conf.setLokiBatchSize(loki_node[hebpf::daemon::CONFIGS_LOKI_BATCH_SIZE].as<int>());
      }
      if (loki_node[hebpf::daemon::CONFIGS_LOKI_FLUSH_INTERVAL]) {
        conf.setLokiFlushInterval(loki_node[hebpf::daemon::CONFIGS_LOKI_FLUSH_INTERVAL].as<int>());
      }
    }
    if (node[hebpf::daemon::CONFIGS_EBPFSO]) {
      conf.setEbpfs(node[hebpf::daemon::CONFIGS_EBPFSO].as<hebpf::daemon::Configs::EbpfMap>());
    }
    return true;
  }
};

} // namespace YAML
