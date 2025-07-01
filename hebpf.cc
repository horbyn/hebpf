// clang-format off
#include <cstdlib>
#include <exception>
#include <iostream>
#include "src/log/logger.h"
#include "src/cmdline/cmd_parser.h"
#include "src/except/except.h"
// clang-format on

auto main(int argc, char **argv) -> int {
  using namespace hebpf;

  try {
    cmdline::CommandParser parser{std::string{log::LOGNAME_DEFAULT}};
    auto ret = parser.parse(argc, argv);
    if (ret != 0) {
      return EXIT_FAILURE;
    }

    HEBPF_GLOBAL_LOG(trace, "启动 hebpf");
    HEBPF_GLOBAL_LOG(debug, "启动 hebpf");
    HEBPF_GLOBAL_LOG(info, "启动 hebpf");

  } catch (const hebpf::except::Success &suc) {
    return EXIT_SUCCESS;
  } catch (const hebpf::except::Exception &exc) {
    std::cout << "[error] " << exc.getMsg() << "\n";
    return EXIT_FAILURE;
  } catch (const std::exception &exc) {
    std::cout << "[error] " << exc.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
