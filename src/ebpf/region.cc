// clang-format off
#include "region.h"
#include "ebpf_common.h"
#include "src/common/common.h"
#include "src/common/exception.h"
// clang-format on

namespace hebpf {
namespace ebpf {

Region::Region(std::weak_ptr<event::EventBusIf> event_bus)
    : event::EventBusHolder{event_bus}, regions_{std::make_shared<RegionMap>()} {
  auto bus = bus_.lock();
  if (bus == nullptr) {
    throw EXCEPT("event bus is null");
  }
  bus->subscribe<event::ConfigEvent>(event::EventHandler{
      FUNCTION_LINE, [this](const event::EventIf &eve) { onConfigEvent(eve); }});
  bus->subscribe<event::HashFailureEvent>(event::EventHandler{
      FUNCTION_LINE, [this](const event::EventIf &eve) { onHashFailureEvent(eve); }});
}

/**
 * @brief 设置 hook id 对应的区域
 *
 * @param hook_key 键
 * @note 同一绑定类型下可能拥有多种分类，通过 id 来区分
 * @note （如果绑定类型为系统 hook 如 kprobe、tracepoint 等则 id 为零；
 * 如果绑定类型为 NIC hook 如 XDP、TC 等则 id 为 NIC 索引）
 *
 * @return true 分配成功
 * @return false 分配失败
 */
bool Region::allocateRegion(const daemon::HookKey &hook_key) {
  std::lock_guard<std::mutex> lock{mutex_};

  auto iter = (*regions_).find(hook_key);
  if (iter != (*regions_).end()) {
    return true;
  }

  auto hook_type = hook_key.hook;
  auto hook_id = hook_key.hook_id;
  int next_start = MAX_CHAIN_PROGS;

  if (regions_->empty()) {
    next_start = 0;
  } else {
    for (const auto &[_, index] : (*regions_)) {
      if (next_start < index) {
        next_start = index;
      }
    }
    next_start -= MAX_PER_HOOK_PROGS;
  }

  if (next_start + MAX_PER_HOOK_PROGS > MAX_CHAIN_PROGS) {
    LOG(error, "No space in prog_array for hook {} id {}", enumName(hook_type), hook_id);
    return false;
  }

  (*regions_)[hook_key] = next_start;

  LOG(debug, "Allocating region, base {} for hook {} id {}", next_start, enumName(hook_type),
      hook_id);
  return true;
}

/**
 * @brief 更新调试接口配置
 *
 * @param out 配置文件
 */
nlohmann::json Region::getDebugStatus() const {
  decltype(regions_) regions_copy{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    regions_copy = regions_;
  }
  return (*regions_copy);
}

/**
 * @brief 配置事件更新
 * @note Path: ConfigManager > Region
 *
 * @param event 事件对象
 */
void Region::onConfigEvent(const event::EventIf &event) {
  auto map_ptr = event.getMap();
  auto diff_ptr = event.getDiff();

  std::shared_ptr<event::EventBusIf> bus{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    bus = bus_.lock();
    if (bus == nullptr) {
      throw EXCEPT("Event bus is invalid");
    }
  }

  auto fail = false;
  for (auto iter = (*map_ptr).begin(); iter != (*map_ptr).end();) {
    if (!allocateRegion(iter->first)) {
      // region 分配失败，删除整个 ProgList，失败的 kv 对不进入下一步
      iter = (*map_ptr).erase(iter);
      fail = true;
    } else {
      ++iter;
    }
  } // end for()

  if (fail) {
    daemon::Diff::filterByMap(map_ptr, diff_ptr);

    bus->publish(
        event::RegionFailureEvent{map_ptr, diff_ptr}); // 失败的 kv 对返回配置管理器，更新后者的缓存
  } // end if(fail)

  bus->publish(event::RegionEvent{map_ptr, regions_, diff_ptr});
}

/**
 * @brief 内核态哈希表事件失败
 * @note Path: ConfigManager > Region > PinnedMap(hash)
 * @note                       Region < PinnedMap(hash)（失败）
 *
 * @param event 事件
 */
void Region::onHashFailureEvent(const event::EventIf &event) {
  std::shared_ptr<event::EventBusIf> bus{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    bus = bus_.lock();
    if (bus == nullptr) {
      throw EXCEPT("Event bus is invalid");
    }

    auto map_ptr = event.getMap();
    auto &map = *map_ptr;
    for (auto iter = regions_->begin(); iter != regions_->end();) {
      auto iter_find = std::find_if(map.begin(), map.end(), [iter](const auto &map_iter) {
        return iter->first == map_iter.first;
      });
      if (iter_find == map.end()) {
        iter = regions_->erase(iter);
      } else {
        ++iter;
      }
    } // end for()
  }

  bus->publish(event::RegionFailureEvent{event.getMap(), event.getDiff()});
}

} // namespace ebpf
} // namespace hebpf
