// clang-format off
#include "gtest/gtest.h"
#include "src/cmdline/cmd_parser.h"
// clang-format on

using namespace hebpf::cmdline;

TEST(CommandParserTest, Parse) {
  CommandParser cp{"TestHebpf"};

  // return 0: 成功
  {
    const char *argv[] = {"./TestHebpf", "--version"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    EXPECT_ANY_THROW(cp.parse(argc, const_cast<char **>(argv)));
  }

  // return 1: 无效选项
  {
    const char *argv[] = {"./TestHebpf", "--invalid-flag"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    EXPECT_NO_THROW(cp.parse(argc, const_cast<char **>(argv)));
    EXPECT_NE(cp.parse(argc, const_cast<char **>(argv)), 0);
  }
}
