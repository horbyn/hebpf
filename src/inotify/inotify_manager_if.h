#pragma once

// clang-format off
#include <functional>
#include <string_view>
#include "inotify_if.h"
// clang-format on

namespace hebpf {
namespace inotify {

class InotifyManagerIf {
public:
  virtual ~InotifyManagerIf() = default;

  virtual bool watchConfig(std::string_view path, InotifyCb on_change) = 0;
  virtual bool goodbyeConfig(std::string_view path) = 0;
};

} // namespace inotify
} // namespace hebpf
