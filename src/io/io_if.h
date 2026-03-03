#pragma once

// clang-format off
#include <memory>
#include <functional>
#include "src/callback/callback.h"
// clang-format on

namespace hebpf {
namespace io {

using IoCb = common::Callback<void()>;

class IoIf {
public:
  virtual ~IoIf() = default;

  virtual std::shared_ptr<void> addReadCb(int fd, IoCb callback) = 0;
};

} // namespace io
} // namespace hebpf
