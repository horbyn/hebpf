// clang-format off
#include "pinned_map_manager.h"
#include "spdlog/fmt/ranges.h"
#include "src/common/assert.h"
#include "src/common/common.h"
// clang-format on

namespace hebpf {
namespace ebpf {

PinnedMapManager::PinnedMapManager(std::weak_ptr<event::EventBusIf> event_bus)
    : event::EventBusHolder{event_bus}, pinned_hash_{std::make_unique<PinnedHash>()},
      pinned_prog_array_{std::make_unique<PinnedProgArray>()} {
  auto bus = bus_.lock();
  if (bus == nullptr) {
    throw EXCEPT("event bus is null");
  }
  bus->subscribe<event::RegionEvent>(event::EventHandler{
      FUNCTION_LINE, [this](const event::EventIf &eve) { onRegionEvent(eve); }});
  bus->subscribe<event::EbpfEvent>(
      event::EventHandler{FUNCTION_LINE, [this](const event::EventIf &eve) { onEbpfEvent(eve); }});
  bus->subscribe<event::EbpfFailureEvent>(event::EventHandler{
      FUNCTION_LINE, [this](const event::EventIf &eve) { onEbpfFailureEvent(eve); }});

  if (pinned_hash_ == nullptr || pinned_prog_array_ == nullptr) {
    throw EXCEPT("pinned map manager init failed");
  }
  if (!pinned_hash_->init()) {
    throw EXCEPT("pinned hash map init failed");
  }
  if (!pinned_prog_array_->init()) {
    throw EXCEPT("pinned prog array map init failed");
  }
}

/**
 * @brief 更新调试接口配置
 *
 * @param out 配置文件
 */
nlohmann::json PinnedMapManager::getDebugStatus() const {
  ASSERT(pinned_hash_ != nullptr);
  ASSERT(pinned_prog_array_ != nullptr);

  return nlohmann::json{{JKEY_KERN_HASH, pinned_hash_->getDebugStatus()},
                        {JKEY_KERN_PRGOARR, pinned_prog_array_->getDebugStatus()}};
}

/**
 * @brief 获取 region 的起始索引
 *
 * @param region RegionMap 对象
 * @param hook_key 键
 * @return int 起始索引，出错返回 -1
 */
int PinnedMapManager::getStartIndex(const RegionMap &region,
                                    const daemon::HookKey &hook_key) const {
  auto iter = region.find(hook_key);
  if (iter == region.end()) {
    return -1;
  }
  return iter->second;
}

/**
 * @brief Region 事件到达
 * @note Path: ConfigManager > Region > PinnedMap(hash)
 *
 * @param event 事件
 */
void PinnedMapManager::onRegionEvent(const event::EventIf &event) {
  ASSERT(pinned_hash_ != nullptr);

  std::shared_ptr<event::EventBusIf> bus{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    bus = bus_.lock();
    if (bus == nullptr) {
      throw EXCEPT("Event bus is invalid");
    }
  }

  const auto *event_ptr = dynamic_cast<const event::RegionEvent *>(&event);
  if (event_ptr == nullptr) {
    // 转换失败，所有配置全部丢弃
    auto map = event.getMap();
    auto diff = event.getDiff();
    map->clear();
    diff->clear();
    bus->publish(event::HashFailureEvent{map, diff});
  }

  auto hook_map_ptr = event_ptr->getMap();
  auto region_map_ptr = event_ptr->getRegionMap();
  auto &hook_map = *hook_map_ptr;
  auto &region_map = *region_map_ptr;
  auto fail = false;

  for (auto iter = hook_map.begin(); iter != hook_map.end();) {
    auto start_index = getStartIndex(region_map, iter->first);
    if (start_index == -1) {
      // 程序数组起始下标丢失后，整个 ProgList 都要删除
      iter = hook_map.erase(iter); // 失败的 kv 对不进入下一步
      fail = true;
      continue;
    } // end if(start_index)

    auto origin_amount = iter->second.size();
    auto origin_progs = getProgNames(iter->second);
    if (!pinned_hash_->updateMapElem(iter->first, start_index, iter->second)) {
      iter = hook_map.erase(iter);
      fail = true;
      continue;
    }

    auto new_amount = iter->second.size();
    auto new_progs = getProgNames(iter->second);
    if (origin_amount != new_amount) {
      LOG(info, "Update IDHASH map: success({}), total({})", fmt::join(new_progs, ","),
          fmt::join(origin_progs, ","));
    }

    fail = false;
    ++iter;
  } // end for()

  auto diff_ptr = event_ptr->getDiff();
  if (fail) {
    daemon::Diff::filterByMap(hook_map_ptr, diff_ptr);

    bus->publish(event::HashFailureEvent{hook_map_ptr, diff_ptr});
  } // end if(fail)

  bus->publish(event::HashEvent{std::move(hook_map_ptr), diff_ptr});
}

/**
 * @brief Ebpf 事件到达
 * @note Path: ConfigManager > Region > PinnedMap(hash) > EbpfManager > PinnedMap(array)
 *
 * @param event 事件对象
 */
void PinnedMapManager::onEbpfEvent(const event::EventIf &event) {
  ASSERT(pinned_hash_ != nullptr);
  ASSERT(pinned_prog_array_ != nullptr);

  std::shared_ptr<event::EventBusIf> bus{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    bus = bus_.lock();
    if (bus == nullptr) {
      throw EXCEPT("Event bus is invalid");
    }
  }

  const auto *event_ptr = dynamic_cast<const event::EbpfEvent *>(&event);
  if (event_ptr == nullptr) {
    // 转换失败，所有配置全部丢弃
    auto map = event.getMap();
    auto diff = event.getDiff();
    map->clear();
    diff->clear();
    bus->publish(event::ProgArrayFailureEvent{map, diff});
  }

  auto hook_map_ptr = event_ptr->getMap();
  auto &hook_map = *hook_map_ptr;
  auto diff_ptr = event_ptr->getDiff();
  if (!pinned_prog_array_->updateMap(hook_map, pinned_hash_.get(), event_ptr->getServiceMap())) {
    daemon::Diff::filterByMap(hook_map_ptr, diff_ptr);
    bus->publish(event::ProgArrayFailureEvent{hook_map_ptr, diff_ptr});
  }

  bus->publish(event::CompleteEvent{hook_map_ptr});
}

/**
 * @brief Ebpf 失败事件到达
 * @note Path: ConfigManager > Region > PinnedMap(Hash) > EbpfManager > PinnedMap(Array)
 * @note                                PinnedMap(Hash) < EbpfManager < PinnedMap(Array) 失败
 *
 * @param event 事件对象
 */
void PinnedMapManager::onEbpfFailureEvent(const event::EventIf &event) {
  ASSERT(pinned_hash_ != nullptr);

  auto hook_map_ptr = event.getMap();
  auto &hook_map = *hook_map_ptr;
  if (!pinned_hash_->rollback(hook_map)) {
    throw EXCEPT("Rollback failed in PinnedMap(Hash)");
  }

  std::shared_ptr<event::EventBusIf> bus{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    bus = bus_.lock();
    if (bus == nullptr) {
      throw EXCEPT("Event bus is invalid");
    }
  }

  bus->publish(event::HashFailureEvent{hook_map_ptr, event.getDiff()});
}

/**
 * @brief 获取程序数组中所有程序的名称
 *
 * @param prog_list 程序数组
 * @return std::vector<std::string> 名称数组
 */
std::vector<std::string> PinnedMapManager::getProgNames(const daemon::ProgList &prog_list) const {
  std::vector<std::string> result{};
  for (const auto &prog : prog_list) {
    result.push_back(prog->getName());
  }
  return result;
}

} // namespace ebpf
} // namespace hebpf
