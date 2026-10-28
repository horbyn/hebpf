#pragma once

// clang-format off
#include "configs.hpp"
// clang-format on

namespace hebpf {
namespace daemon {

/**
 * @brief 比较 eBPF yaml 配置对象列表
 *
 * @param lhs 列表 A（常量，不会修改）
 * @param rhs 列表 B（常量，不会修改）
 * @param comp 比较函数
 * @param ignore 忽略函数
 * @return true 相同
 * @return false 不相同
 */
template <typename Comparator, typename IgnorePred>
bool Configs::compareEbpfMap(const Configs::EbpfMap &lhs, const Configs::EbpfMap &rhs,
                             Comparator comp, IgnorePred ignore) {
  auto it_l = lhs.begin();
  auto it_r = rhs.begin();
  while (it_l != lhs.end() || it_r != rhs.end()) {
    while (it_l != lhs.end() && ignore(it_l->first, *it_l->second)) {
      ++it_l;
    }
    while (it_r != rhs.end() && ignore(it_r->first, *it_r->second)) {
      ++it_r;
    }

    // 如果都到末尾，说明所有非忽略条目比较完毕且相等
    if (it_l == lhs.end() && it_r == rhs.end()) {
      return true;
    }
    // 其中一方先结束，说明数量不匹配
    if (it_l == lhs.end() || it_r == rhs.end()) {
      return false;
    }

    if (it_l->first != it_r->first) {
      return false;
    }
    if (it_l->second == nullptr || it_r->second == nullptr) {
      return false;
    }

    if (!comp(*it_l->second, *it_r->second)) {
      return false;
    }
    ++it_l;
    ++it_r;
  }
  return true;
}

/**
 * @brief 遍历 eBPF yaml 配置对象列表
 *
 * @param map 列表（常量，不会修改）
 * @param func 遍历函数
 * @param ignore 忽略函数
 */
template <typename Func, typename IgnorePred>
void Configs::traversalEbpfMap(const EbpfMap &map, Func func, IgnorePred ignore) {
  for (auto iter = map.begin(); iter != map.end(); ++iter) {
    if (ignore(iter->first, *iter->second)) {
      continue;
    }
    func(iter->first, *iter->second);
  }
}

} // namespace daemon
} // namespace hebpf
