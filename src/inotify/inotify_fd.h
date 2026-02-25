#pragma once

// clang-format off
#include "src/fd/fd.h"
// clang-format on

namespace hebpf {
namespace inotify {

class InotifyFd : public Fd {
public:
  explicit InotifyFd();
};

} // namespace inotify
} // namespace hebpf
