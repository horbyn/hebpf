// clang-format off
#include "monitor_factory.h"
#include "src/common/assert.h"
#include "monitor.h"
// clang-format on

namespace hebpf {
namespace monitor {

/**
 * @brief 创建 Monitor 对象
 *
 * @param bind_address 监听地址
 * @return std::shared_ptr<MonitorIf> 对象
 */
std::shared_ptr<MonitorIf> MonitorFactory::create(std::string_view bind_address) {
  ASSERT(!bind_address.empty());

  auto bind_address_str = std::string{bind_address};
  std::lock_guard<std::mutex> lock{mutex_};
  auto iter = cache_.find(bind_address_str);
  if (iter != cache_.end()) {
    auto shared = iter->second.lock();
    if (shared) {
      return shared;
    }
  }

  auto monitor = std::make_shared<Monitor>(bind_address);
  cache_[bind_address_str] = monitor;
  return monitor;
}

} // namespace monitor
} // namespace hebpf
