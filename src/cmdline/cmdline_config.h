#pragma once

// clang-format off
#include <string>
#include "src/daemon/configs.hpp"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace cmdline {

class CmdlineConfig final {
public:
  bool help_{false};
  daemon::Configs config_{};
  bool gen_empty_{false};
  std::string log_{log::LOGFILE_DEFAULT};
  log::Level loglevel_{log::LOGLEVEL_DEFAULT};
};

} // namespace cmdline
} // namespace hebpf
