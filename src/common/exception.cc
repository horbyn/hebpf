// clang-format off
#include "exception.h"
#include <cstring>
#include <errno.h>
#include <memory>
#include "spdlog/spdlog.h"
#include "src/common/stack_frame.h"
// clang-format on

namespace hebpf {
namespace except {

/**
 * @brief 构造异常对象
 * @param file: 使用宏 __FILE__
 * @param line: 使用宏 __LINE__
 * @param msg:  提示信息
 * @param is_errno: 是否从 errno 中获取错误信息
 */
Exception::Exception(std::string_view file, int line, std::string_view msg, bool is_errno) {
  stack_ = stackTrace();
  err_ = errno;
  msg_ = fmt::format("[{}:{}] {}", file, line, msg);

  if (is_errno && err_ != 0) {
    constexpr int MAXSIZE = 256;
    std::unique_ptr<char[]> err_msg = std::make_unique<char[]>(MAXSIZE);
    if (::strerror_r(err_, err_msg.get(), MAXSIZE) == 0) {
      msg_ = fmt::format("{} ({} -- {})", msg_, err_, err_msg.get());
    }
  }
}

/**
 * @brief 获取出错信息
 *
 * @return std::string 出错信息
 */
const char *Exception::what() const noexcept { return msg_.c_str(); }

/**
 * @brief 返回栈帧
 *
 * @return std::string 栈帧
 */
std::string Exception::stackFrame() const noexcept { return stack_; }

} // namespace except
} // namespace hebpf
