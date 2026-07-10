#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {
namespace debug_server {

class Root final {
public:
  explicit Root() = default;
  explicit Root(std::string_view vers, std::string_view bl_time, const nlohmann::json &info);

  void setVersion(std::string_view vers);
  std::string getVersion() const;

  void setBuildTime(std::string_view bl_time);
  std::string getBuildTime() const;

  void setInfo(const nlohmann::json &info);
  nlohmann::json getInfo() const;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Root, version, build_time, info);

private:
  std::string version;
  std::string build_time;
  nlohmann::json info;
};

} // namespace debug_server
} // namespace hebpf
