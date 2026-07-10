// clang-format off
#include "debug_server_root.h"
// clang-format on

namespace hebpf {
namespace debug_server {

Root::Root(std::string_view vers, std::string_view bl_time, const nlohmann::json &info)
    : version{}, build_time{}, info{} {
  setVersion(vers);
  setBuildTime(bl_time);
  setInfo(info);
}

/**
 * @brief 设置版本号
 *
 * @param vers 版本号
 */
void Root::setVersion(std::string_view vers) { version = std::string{vers}; }

/**
 * @brief 获取版本号
 *
 * @return std::string 版本号
 */
std::string Root::getVersion() const { return version; }

/**
 * @brief 设置构建时间
 *
 * @param bl_time 时间
 */
void Root::setBuildTime(std::string_view bl_time) { build_time = std::string{bl_time}; }

/**
 * @brief 获取构建时间
 *
 * @return std::string 构建时间
 */
std::string Root::getBuildTime() const { return build_time; }

/**
 * @brief 设置 debug 信息
 *
 * @param info 信息
 */
void Root::setInfo(const nlohmann::json &info) { this->info = info; }

/**
 * @brief 获取 debug 信息
 *
 * @return nlohmann::json 信息
 */
nlohmann::json Root::getInfo() const { return info; }

} // namespace debug_server
} // namespace hebpf
