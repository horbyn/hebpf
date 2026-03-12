// clang-format off
#include "cmdline.h"
#include <vector>
#include "spdlog/fmt/ranges.h"
#include "src/common/enum_name.hpp"
// clang-format on

namespace hebpf {
namespace cmdline {

cmdline::Cmdline::Cmdline() : app_{std::string{APP_DESCRIPTION}} { setup(); }

/**
 * @brief 解析命令行参数
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数选项
 * @return std::pair<CmdlineConfig, bool> 配置对象和解析是否成功
 */
std::pair<CmdlineConfig, bool> Cmdline::parse(int argc, char **argv) {
  try {
    app_.parse(argc, argv);

    if (config_path_.empty()) {
      config_path_ = std::string{daemon::CONFIGS_DEFAULT};
    }
    config_.config_ = daemon::Configs::loadFromConfig(config_path_);

    auto level_opt = stringEnum<log::Level>(loglevel_str_);
    if (level_opt.has_value()) {
      config_.loglevel_ = level_opt.value();
    }

    config_.gen_empty_ = gen_empty_;
    config_.help_ = false;
    if (!logfile_str_.empty()) {
      config_.log_ = logfile_str_;
    }

    return {config_, true};
  } catch (const CLI::ParseError &perr) {
    auto ret = app_.exit(perr);
    return ret == 0 ? std::make_pair<CmdlineConfig, bool>(CmdlineConfig{.help_ = true}, true)
                    : std::make_pair<CmdlineConfig, bool>(CmdlineConfig{}, false);
  } catch (const std::exception &exc) {
    LOG(error, "Unexpected error in cmdline stage: {}", exc.what());
    return {CmdlineConfig{}, false};
  }
}

/**
 * @brief 获取 usage
 *
 * @return std::string 信息
 */
std::string Cmdline::getHelp() const { return app_.help(); }

/**
 * @brief 设置命令行选项
 *
 */
void Cmdline::setup() {
  // 配置文件
  app_.add_option("-c,--config", config_path_, "配置文件路径")
      ->default_str(std::string{daemon::CONFIGS_DEFAULT})
      ->check(CLI::ExistingFile);

  // 生成空的配置文件
  app_.add_flag("--get-conf", gen_empty_, "生成空的配置文件然后退出");

  // 日志等级
  auto info_vec = enumNameList<log::Level>();
  info_vec.pop_back(); // MAXSIZE
  std::string info_desc = fmt::format("日志等级 ({})", fmt::join(info_vec, "/"));
  app_.add_option("--log-level", loglevel_str_, info_desc)
      ->default_str(std::string{enumName(log::LOGLEVEL_DEFAULT)})
      ->check(CLI::IsMember(info_vec));

  // 日志文件
  app_.add_option("-l,--log-file", logfile_str_, "日志文件路径")
      ->default_str(std::string{log::LOGFILE_DEFAULT});

  // usage
  app_.set_help_flag("-h,--help", "打印 usage 并退出");
}

} // namespace cmdline
} // namespace hebpf
