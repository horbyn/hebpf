#pragma once

// clang-format off
#include <cassert>
#include <cstdlib>
#include <iostream>
// clang-format on

namespace hebpf {

// hebpf 假设失效时行为: Release 直接输出 stderr, Debug 立即终止以便 coredump
#ifdef NDEBUG
#define ASSERT_ACTION()                                                                            \
  std::cerr << "hebpf assertion failed in " << (__FILE__) << ":" << (__LINE__) << "\n"
#else
#define ASSERT_ACTION() std::abort()
#endif

// hebpf 运行时假设性检查
#define ASSERT(expr)                                                                               \
  do {                                                                                             \
    assert((expr));                                                                                \
    if (!(expr)) {                                                                                 \
      ASSERT_ACTION();                                                                             \
    }                                                                                              \
  } while (0)

// hebpf 编译时假设性检查
#define STATIC_ASSERT(expr, comment) static_assert((expr), comment)

} // namespace hebpf
