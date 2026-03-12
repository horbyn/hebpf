#pragma once

// clang-format off
#include <optional>
#include <string>
#include <vector>
#include "magic_enum/magic_enum.hpp"
// clang-format on

namespace hebpf {

/**
 * @brief 枚举值转换为字符串
 *
 * @param e 枚举值
 * @return constexpr std::string_view 对应字符串
 */
template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
constexpr std::string_view enumName(Enum e) noexcept {
  return magic_enum::enum_name(e);
}

/**
 * @brief 枚举值转换字符串数组
 *
 * @return std::vector<std::string> 字符串数组
 */
template <typename Enum>
std::vector<std::string> enumNameList() {
  std::vector<std::string> names{};
  for (auto value : magic_enum::enum_values<Enum>()) {
    names.emplace_back(std::string{enumName(value)});
  }
  return names;
}

/**
 * @brief 字符串转换为枚举值
 *
 * @param str 字符串
 * @return constexpr std::optional<Enum> 枚举值
 */
template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
constexpr std::optional<Enum> stringEnum(std::string_view str) noexcept {
  return magic_enum::enum_cast<Enum>(str);
}

} // namespace hebpf
