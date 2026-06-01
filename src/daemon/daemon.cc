// clang-format off
#include "daemon.h"
#include <pthread.h>
#include <algorithm>
#include <unordered_set>
#include "loader.h"
#include "src/common/assert.h"
#include "src/common/exception.h"
#include "src/thread/thread.h"
// clang-format on

namespace hebpf {
namespace daemon {

Daemon::Daemon(std::unique_ptr<LoaderIf> loader) : loader_{std::move(loader)} {}

/**
 * @brief 运行 deamon
 *
 */
void Daemon::run() {
  if (running_.load()) {
    return;
  }

  running_.store(true);

  {
    std::lock_guard<std::mutex> lock{queue_mutex_};
    if (status_queue_ != nullptr) {
      producer_thread_ = std::make_unique<thread::Thread>();
      producer_thread_->setName(NAME_DAEMON_PROD);
      producer_thread_->start(std::bind(&Daemon::produceLoop, this));
    }
  }
}

/**
 * @brief 停止 daemon
 *
 */
void Daemon::stop() {
  if (!running_.load()) {
    return;
  }

  running_.store(false);

  if (loader_ != nullptr) {
    const auto &all_ebpf = loader_->getAllService();
    for (const auto &so_name : all_ebpf) {
      unloadEbpf(so_name);
    }
    current_ebpf_.clear();
  }
  LOG(info, "eBPF programs stopped");

  {
    std::lock_guard<std::mutex> lock{queue_mutex_};
    if (producer_thread_ != nullptr && producer_thread_->joinable()) {
      producer_thread_->join();
      producer_thread_.reset();
    }
    if (status_queue_ != nullptr) {
      status_queue_->stop();
      status_queue_.reset();
    }
  }
}

/**
 * @brief 更新调试接口配置
 *
 * @param out 配置文件
 */
void Daemon::update(nlohmann::json &out) {
  nlohmann::json obj{};
  obj["running"] = running_.load();

  {
    std::lock_guard<std::mutex> lock{queue_mutex_};
    if (status_queue_ != nullptr) {
      obj["queue_empty"] = status_queue_->empty();
      obj["queue_full"] = status_queue_->full();
    }

    if (loader_ != nullptr) {
      obj["Loader"] = loader_->getDebugStatus();
    }
  }

  out["Daemon"] = std::move(obj);
}

/**
 * @brief 更新配置文件
 *
 * @param config 配置文件对象
 */
void Daemon::update(const Configs &config) {
  ASSERT(loader_ != nullptr);
  auto new_ebpf = config.getEbpfs();

  decltype(new_ebpf) current_ebpf_with_scd{};
  daemon::LoaderIf *loader{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    current_ebpf_with_scd = current_ebpf_;
    loader = loader_.get();
  }

  auto current_ebpf_without_scd{current_ebpf_with_scd};
  for (auto iter = current_ebpf_without_scd.begin(); iter != current_ebpf_without_scd.end();) {
    if (iter->first == fmt::format(ID_SCHEDULER_REGEX, enumName(iter->second.getHook()),
                                   iter->second.getIfindex())) {
      // 跳过 scheduler
      iter = current_ebpf_without_scd.erase(iter);
    } else {
      ++iter;
    }
  }

  if (new_ebpf != current_ebpf_without_scd) {
    // 重建程序链顺序
    std::unordered_map<HookType, std::vector<std::tuple<int, std::string, int>>> hook_order_temp{};
    for (const auto &[_, cfg] : new_ebpf) {
      auto canonical_name = ebpf::PinnedProgMap::getCanonicalName(
          cfg.getHook(), ebpf::PinnedProgMap::UnofficialName::LIBRARY, cfg.getLib());
      hook_order_temp[cfg.getHook()].emplace_back(cfg.getOrder(), canonical_name, cfg.getIfindex());
    }

    std::unordered_map<HookType, ebpf::PinnedProgMap::NamesIfindexVec> hook_order{};
    for (auto &[hook, vec] : hook_order_temp) {
      std::sort(vec.begin(), vec.end(),
                [](const auto &a, const auto &b) { return std::get<0>(a) < std::get<0>(b); });
      ebpf::PinnedProgMap::NamesIfindexVec names{};
      names.reserve(vec.size());
      for (const auto &[_, name, ifindex] : vec) {
        names.emplace_back(name, ifindex);
      }
      hook_order[hook] = std::move(names);
    }

    // 调用 Loader 更新链
    for (const auto &[hook, vec] : hook_order) {
      for (const auto &pair : vec) {
        loader->allocateChain(hook, pair.second);
      }
      loader->updateProgChain(hook, vec);
    }
  } // end if(重建程序链)

  // 需要卸载的
  for (const auto &elem : current_ebpf_without_scd) {
    if (new_ebpf.find(elem.first) == new_ebpf.end()) {
      auto lib = elem.second.getLib();
      unloadEbpf(lib);
    }
  }

  // 需要为不同 hook 类型且不同网卡的配置都创建一个 Scheduler：
  // 1. 如出现 LB XDP NIC11 和 LB XDP NIC12，那么就需要创建两个 Scheduler
  //   然后绑定到 NIC11 和 NIC12 的 XDP 上
  // 2. 再如出现 LB XDP NIC11 和 ACL ACL NIC11，那么也需要创建两个 Scheduler
  //   然后绑定到 NIC11 的 XDP 和 ACL 上
  // 3. 再如出现 LB XDP NIC11 和 ACL XDP NIC11，那么只需要创建一个 Scheduler
  //   然后绑定到 NIC11 的 XDP 上
  std::set<std::pair<HookType, int>> needed_scheduler_pairs{};
  for (const auto &[name, cfg] : new_ebpf) {
    // 链表里有多少个元素就创建多少个 Scheduler
    needed_scheduler_pairs.insert({cfg.getHook(), cfg.getIfindex()});
  }

  for (const auto &[hook, ifidx] : needed_scheduler_pairs) {
    auto so_path = hook == daemon::HookType::TC ? std::string{ebpf::SCHEDULER_TC_LIB}
                                                : std::string{ebpf::SCHEDULER_XDP_LIB};
    // 这里 so_path 要特殊处理一下，因为 Loader 对象以这个 so_path 作为哈希表的 key
    //    要支持多个 Schduler，则每个 key 需要唯一（格式 xdpgen_11, tc_11 区分）
    so_path = fmt::format("{};{}_{}", so_path, enumName(hook), ifidx);
    std::string id = fmt::format(ID_SCHEDULER_REGEX, enumName(hook), ifidx);
    // 向新的配置中添加 Scheduler，key 格式为 scheduler;xdpgen_11, scheduler;tc_11
    new_ebpf.emplace(id, ConfigEbpf{so_path, std::string{}, hook, ifidx, 0});
  }

  // 需要加载的
  for (const auto &elem : new_ebpf) {
    auto new_lib = elem.second.getLib();
    auto new_conf = elem.second.getConfig();
    auto new_hook = elem.second.getHook();
    auto new_ifindex = elem.second.getIfindex();

    auto current_it = current_ebpf_with_scd.find(elem.first);
    if (current_it == current_ebpf_with_scd.end()) {
      // 加载
      loadEbpf(new_lib, new_hook, new_ifindex, new_conf);
    } else {
      // 更新
      auto current_lib = current_it->second.getLib();
      auto current_hook = current_it->second.getHook();
      auto current_ifindex = current_it->second.getIfindex();
      if (current_lib != new_lib || current_hook != new_hook || current_ifindex != new_ifindex) {
        unloadEbpf(current_lib);
        loadEbpf(new_lib, new_hook, new_ifindex, new_conf);
        continue; // 更新动态库同时也会更新配置文件，所以后续不需再继续检查配置文件了
      }

      auto current_conf = current_it->second.getConfig();
      if (current_conf != new_conf) {
        loader->unregisterServiceConfig(current_lib, current_conf);
        loader->registerServiceConfig(new_lib, new_hook, new_ifindex, new_conf);
      }
    }
  }

  {
    std::lock_guard<std::mutex> lock{mutex_};
    current_ebpf_ = new_ebpf;
  }
}

/**
 * @brief 设置单生产者-单消费者队列
 *
 * @param queue
 */
void Daemon::setStatusQueue(std::shared_ptr<QueueDaemonMonitor> queue) { status_queue_ = queue; }

/**
 * @brief Daemon 生产者事件循环
 *
 */
void Daemon::produceLoop() noexcept {
  constexpr int sleep_time_sec = 1;

  try {
    while (running_.load()) {
      Configs::EbpfMap services{};

      {
        std::lock_guard<std::mutex> lock{mutex_};
        services = current_ebpf_;
      }

      for (const auto &pair : services) {
        auto lib = pair.second.getLib();
        const auto *service = loader_->getService(lib);
        if (service != nullptr) {
          auto status = service->getPrometheusStatus();
          if (!status.empty()) {
            std::shared_ptr<QueueDaemonMonitor> queue{};
            {
              std::lock_guard<std::mutex> lock{queue_mutex_};
              queue = status_queue_;
            }
            if (queue != nullptr && !queue->full()) {
              queue->push(status);
            }
          }
        }
      } // end for()

      std::this_thread::sleep_for(std::chrono::seconds(sleep_time_sec));
    } // end while()
  } catch (const except::Exception &exc) {
    LOG(error, "Daemon productor dropped: {}\nDaemon stackframe:\n{}", exc.what(),
        exc.stackFrame());
  } catch (const std::exception &exc) {
    LOG(error, "Daemon productor dropped: {}", exc.what());
  }
}

/**
 * @brief 加载 eBPF 程序
 *
 * @param so_path eBPF 动态库路径
 * @param hook_type hook 类型
 * @param ifindex 网卡索引
 * @param config_path eBPF 配置路径
 */
void Daemon::loadEbpf(std::string_view so_path, HookType hook_type, int ifindex,
                      std::string_view config_path) {
  ASSERT(!so_path.empty());
  ASSERT(loader_ != nullptr);

  std::weak_ptr<io::IoIf> io_ctx{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!loader_->loadService(so_path, hook_type, ifindex, config_path)) {
      LOG(error, "Cannot load service: {} on interface {}", so_path, ifindex);
      return;
    }
  }
}

/**
 * @brief 卸载 eBPF 程序
 *
 * @param so_path eBPF 动态库路径
 */
void Daemon::unloadEbpf(std::string_view so_path) {
  ASSERT(!so_path.empty());
  ASSERT(loader_ != nullptr);

  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!loader_->unloadServices(so_path)) {
      LOG(error, "Cannot unload service: {}", so_path);
    }
  }
}

} // namespace daemon
} // namespace hebpf
