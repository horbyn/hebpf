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
constexpr std::string_view CONFIGS_LOGPATH{"logpath"};
constexpr std::string_view CONFIGS_LOGLEVEL{"loglevel"};
constexpr std::string_view CONFIGS_EBPFSO{"ebpf"};

class Configs final : public log::Loggable<log::Id::daemon> {
public:
  Configs() = default;
  Configs(std::string_view configs_path);

  static Configs loadFromConfig(std::string_view filepath);

  void setLog(std::string_view logpath);
  std::string getLog() const;
  void setLogLevel(log::Level level);
  log::Level getLogLevel() const;
  void setEbpf(const std::vector<std::string> &ebpf_so);
  std::vector<std::string> getEbpf() const;

  bool operator==(const Configs &other) const;
  bool operator!=(const Configs &other) const;

private:
  std::string log_{log::LOGFILE_DEFAULT};
  log::Level loglevel_{log::Level::debug};
  std::vector<std::string> ebpf_;
};

} // namespace daemon
} // namespace hebpf

namespace YAML {

template <>
struct convert<hebpf::daemon::Configs> {
  static Node encode(const hebpf::daemon::Configs &conf) {
    Node node{};
    node[hebpf::daemon::CONFIGS_LOGPATH] = conf.getLog();
    node[hebpf::daemon::CONFIGS_LOGLEVEL] = hebpf::enumName(conf.getLogLevel());
    auto vec = conf.getEbpf();
    if (!vec.empty()) {
      node[hebpf::daemon::CONFIGS_EBPFSO] = conf.getEbpf();
    }
    return node;
  }

  static bool decode(const Node &node, hebpf::daemon::Configs &conf) {
    if (node[hebpf::daemon::CONFIGS_LOGPATH]) {
      conf.setLog(node[hebpf::daemon::CONFIGS_LOGPATH].as<std::string>());
    }
    if (node[hebpf::daemon::CONFIGS_LOGLEVEL]) {
      auto level_opt = hebpf::stringEnum<hebpf::log::Level>(
          node[hebpf::daemon::CONFIGS_LOGLEVEL].as<std::string>());
      auto level = level_opt.has_value() ? level_opt.value() : hebpf::log::Level::info;
      conf.setLogLevel(level);
    }
    if (node[hebpf::daemon::CONFIGS_EBPFSO]) {
      conf.setEbpf(node[hebpf::daemon::CONFIGS_EBPFSO].as<std::vector<std::string>>());
    }
    return true;
  }
};

} // namespace YAML
