#pragma once

// clang-format off
#include <memory>
#include <string>
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace cmdline {

class CommandParserIf {
public:
  virtual ~CommandParserIf() = default;
  virtual auto parse(int argc, char **argv) -> int = 0;
};

class CommandParser : public CommandParserIf, public log::Loggable<log::Id::cmdline> {
public:
  explicit CommandParser(const std::string &app_name);
  auto parse(int argc, char **argv) -> int override;

private:
  std::string prog_name_;
};

} // namespace cmdline
} // namespace hebpf
