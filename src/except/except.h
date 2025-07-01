#pragma once

// clang-format off
#include <stdexcept>
// clang-format on

namespace hebpf {
namespace except {

class Exception : public std::exception {
public:
  Exception() = default;
  virtual ~Exception() = default;
  Exception(const std::string &file, int line, const std::string &msg, bool is_errno);

  auto getMsg() const -> std::string;

private:
  std::string msg_;
  int err_;
};

class Success : public Exception {
public:
  virtual ~Success() = default;
};

} // namespace except

#define EXHEBPF(msg, is_errno) (except::Exception((__FILE__), (__LINE__), (msg), (is_errno)))

} // namespace hebpf
