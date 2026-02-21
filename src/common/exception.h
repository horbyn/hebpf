#pragma once

// clang-format off
#include <stdexcept>
#include <string>
#include <string_view>
// clang-format on

namespace hebpf {
namespace except {

class Exception : public std::exception {
public:
  virtual ~Exception() = default;
  explicit Exception(std::string_view file, int line, std::string_view msg, bool is_errno);

  const char *what() const noexcept;
  std::string stackFrame() const noexcept;

protected:
  std::string msg_;
  int err_;
  std::string stack_;
};

} // namespace except

#define EXCEPT1(msg) (except::Exception((__FILE__), (__LINE__), (msg), false))
#define EXCEPT2(msg, is_errno) (except::Exception((__FILE__), (__LINE__), (msg), (is_errno)))
#define GET_EXCEPT_MACRO(_1, _2, NAME, ...) NAME
#define EXCEPT(...) GET_EXCEPT_MACRO(__VA_ARGS__, EXCEPT2, EXCEPT1)(__VA_ARGS__)

} // namespace hebpf
