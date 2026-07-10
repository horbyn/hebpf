#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"
#include "src/ebpf/pinned_prog_map_data.h"
// clang-format on

namespace hebpf {
namespace daemon {

class ServiceJson final {
public:
  explicit ServiceJson() = default;
  explicit ServiceJson(std::string_view so_path, std::string_view name,
                       std::string_view config_path, std::string_view hook, int ifindex,
                       const nlohmann::json &status);

  void setSoPath(std::string_view so_path);
  std::string getSoPath() const;

  void setName(std::string_view name);
  std::string getName() const;

  void setConfigPath(std::string_view config_path);
  std::string getConfigPath() const;

  void setHook(std::string_view hook);
  std::string getHook() const;

  void setIfindex(int ifindex);
  int getIfindex() const;

  void setStatus(const nlohmann::json &status);
  nlohmann::json getStatus() const;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ServiceJson, so_path, name, config_path, hook,
                                              ifindex, status);

private:
  std::string so_path;
  std::string name;
  std::string config_path;
  std::string hook;
  int ifindex;
  nlohmann::json status;
};

class LoaderJson final {
public:
  explicit LoaderJson() = default;
  explicit LoaderJson(const std::vector<ServiceJson> &services,
                      const std::vector<ebpf::EbpfProgMap> &pinned_map);

  void appendService(const ServiceJson &service);
  void appendPinnedMap(const ebpf::EbpfProgMap &pinned_map);

  NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(LoaderJson, services, pinned_map);

private:
  std::vector<ServiceJson> services;
  std::vector<ebpf::EbpfProgMap> pinned_map;
};

} // namespace daemon
} // namespace hebpf
