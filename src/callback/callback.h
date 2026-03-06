#pragma once

// clang-format off
#include <functional>
#include <utility>
#include <string>
// clang-format on

namespace hebpf {
namespace common {

template <typename T>
class Callback;

template <typename Ret, typename... Args>
class Callback<Ret(Args...)> {
public:
  using FuncType = std::function<Ret(Args...)>;

  explicit Callback() = default;
  explicit Callback(std::string name, FuncType func);

  Ret operator()(Args... args);
  std::string name() const;

  explicit operator bool() const noexcept;

private:
  std::string name_;
  FuncType func_;
};

} // namespace common
} // namespace hebpf

#include "callback.tpp"
