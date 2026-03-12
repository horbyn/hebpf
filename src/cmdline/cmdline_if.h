#pragma once

// clang-format off
#include <string>
#include <utility>
#include "src/daemon/configs.hpp"
#include "cmdline_config.h"
// clang-format on

namespace hebpf {
namespace cmdline {

class CmdlineIf {
public:
  virtual ~CmdlineIf() = default;
  virtual std::pair<CmdlineConfig, bool> parse(int argc, char **argv) = 0;
  virtual std::string getHelp() const = 0;
};

} // namespace cmdline
} // namespace hebpf
