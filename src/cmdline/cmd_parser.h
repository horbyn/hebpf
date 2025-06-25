#pragma once

// clang-format off
#include <memory>
#include <string>
#include "CLI/CLI.hpp"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace cmdline {

class CommandParserIf {
public:
  virtual ~CommandParserIf() = default;
  virtual auto parse(int argc, char **argv) noexcept -> int = 0;
};

class CommandParser : public CommandParserIf, public log::Loggable<log::Id::cmdline> {
public:
  explicit CommandParser(const std::string &app_name);

  template <typename T>
  auto add_option(const std::string &name, T &variable, const std::string &description,
                  bool required = false) -> void;
  auto add_flag(const std::string &name, bool &variable, const std::string &description) -> void;
  auto parse(int argc, char **argv) noexcept -> int override;

private:
  CLI::App app_;
  std::vector<std::unique_ptr<CLI::App>> subcommands_;
};

} // namespace cmdline
} // namespace hebpf

#include "cmd_parser.tpp"
