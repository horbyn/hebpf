#pragma once

// clang-format off
#include <string>
#include <string_view>
// clang-format on

namespace hebpf {

std::string demangle(std::string_view symbol);

#define RTTI_NAME(obj) (demangle(typeid(obj).name()))

std::string stackTrace(bool async_signal_safe = false);

} // namespace hebpf
