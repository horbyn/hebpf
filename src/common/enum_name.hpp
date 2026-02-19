#pragma once

// clang-format off
#include <optional>
#include "magic_enum/magic_enum.hpp"
// clang-format on

namespace hebpf {

template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
constexpr std::string_view enumName(Enum e) noexcept {
  return magic_enum::enum_name(e);
}

template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
constexpr std::optional<Enum> stringEnum(std::string_view str) noexcept {
  return magic_enum::enum_cast<Enum>(str);
}

} // namespace hebpf
