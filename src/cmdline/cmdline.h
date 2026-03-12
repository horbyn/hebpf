#pragma once

// clang-format off
#include <string_view>
#include "CLI/CLI.hpp"
#include "src/log/logger.h"
#include "cmdline_if.h"
#include "cmdline_config.h"
// clang-format on

namespace hebpf {
namespace cmdline {

constexpr std::string_view APP_DESCRIPTION{"hebpf - eBPF 程序管理平台"};

class Cmdline : public CmdlineIf, public log::Loggable<log::Id::cmdline> {
public:
  explicit Cmdline();

  std::pair<CmdlineConfig, bool> parse(int argc, char **argv) override;
  std::string getHelp() const override;

private:
  void setup();

  CLI::App app_;

  std::string config_path_;
  bool gen_empty_{false};
  std::string loglevel_str_;
  std::string logfile_str_;

  CmdlineConfig config_;
};

} // namespace cmdline
} // namespace hebpf
