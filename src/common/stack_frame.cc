// clang-format off
#include "stack_frame.h"
#include <cxxabi.h>
#include <execinfo.h>
#include <memory>
// clang-format on

namespace hebpf {

/**
 * @brief 将二进制符号转换为 C++ 函数名
 *
 * @param symbol 符号名
 * @return std::string C++ 函数名（转换失败会返回空字符串）
 */
std::string demangle(std::string_view symbol) {
  int status = 0;
  std::unique_ptr<char, decltype(&std::free)> demangled{
      abi::__cxa_demangle(symbol.data(), nullptr, nullptr, &status), std::free};
  return demangled ? std::string(demangled.get()) : std::string{};
}

/**
 * @brief 将二进制符号转换为 C++ 函数名 wrapper
 *
 * @param symbol 符号名
 * @return std::string C++ 函数名（转换失败会返回空字符串）
 */
static std::string demangleWrapper(std::string_view symbol) {
  static constexpr std::size_t MAXSIZE = 128;
  char temp[MAXSIZE]{};

  // C++ 符号
  if (std::sscanf(symbol.data(), "%*[^(](%127[^)+]", temp) == 1) {
    auto result = demangle(temp);
    if (!result.empty()) {
      return result;
    }
  }

  // C 符号
  if (std::sscanf(symbol.data(), "%127s", temp) == 1) {
    return temp;
  }

  // 其他符号
  return std::string{symbol};
}

/**
 * @brief 回溯栈帧
 *
 * @return std::string 一个包含栈帧的字符串
 */
std::string stackTrace() {
  constexpr std::size_t MAXSIZE = 256;
  std::array<void *, MAXSIZE> buffer{};
  std::string result{};

  int nptrs = ::backtrace(buffer.data(), MAXSIZE);
  std::unique_ptr<char *, void (*)(char **)> strings{::backtrace_symbols(buffer.data(), nptrs),
                                                     [](char **ptr) {
                                                       if (ptr)
                                                         std::free(ptr);
                                                     }};

  if (strings) {
    for (int i = 1; i < nptrs; ++i) {
      result += "[" + std::to_string(i) + "] ";
      result.append(demangleWrapper(strings.get()[i]));
      result.push_back('\n');
    }
  }

  return result;
}

} // namespace hebpf
