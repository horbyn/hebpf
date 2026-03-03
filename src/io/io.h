#pragma once

// clang-format off
#include "io_if.h"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace io {

class Io : public IoIf, public log::Loggable<log::Id::io> {
private:
  class IoContext;

public:
  explicit Io();
  ~Io();

  std::shared_ptr<void> addReadCb(int fd, IoCb callback) override;

private:
  std::unique_ptr<IoContext> impl_;
};

} // namespace io
} // namespace hebpf
