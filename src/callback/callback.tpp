#pragma once

// clang-format off
#include "callback.h"
// clang-format on

namespace hebpf {
namespace common {

template <typename Ret, typename... Args>
hebpf::common::Callback<Ret(Args...)>::Callback(std::string name, FuncType func)
    : name_(std::move(name)), func_(std::move(func)) {}

/**
 * @brief 调用函数
 *
 * @tparam Ret 返回值类型
 * @tparam Args 可变参数
 * @param args 参数
 * @return Ret 返回值
 */
template <typename Ret, typename... Args>
Ret Callback<Ret(Args...)>::operator()(Args... args) {
  return func_(std::forward<Args>(args)...);
}

/**
 * @brief 获取回调函数名称
 *
 * @tparam Ret 返回值类型
 * @tparam Args 可变参数
 * @return std::string 名称
 */
template <typename Ret, typename... Args>
std::string Callback<Ret(Args...)>::name() const {
  return name_;
}

/**
 * @brief 判断回调函数是否为空
 *
 * @tparam Ret 返回值类型
 * @tparam Args 可变参数
 * @return true 非空
 * @return false 空
 */
template <typename Ret, typename... Args>
Callback<Ret(Args...)>::operator bool() const noexcept {
  return static_cast<bool>(func_);
}

} // namespace common
} // namespace hebpf
