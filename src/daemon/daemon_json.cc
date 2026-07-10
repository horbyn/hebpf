// clang-format off
#include "daemon_json.h"
// clang-format on

namespace hebpf {
namespace daemon {

DaemonJson::DaemonJson(bool running, bool queue_empty, bool queue_full, const LoaderJson &loader) {
  setRunning(running);
  setQueueEmpty(queue_empty);
  setQueueFull(queue_full);
  setLoader(loader);
}

/**
 * @brief 设置运行标识
 *
 * @param running 运行标识，true 正在运行；false 停止
 */
void DaemonJson::setRunning(bool running) { this->running = running; }

/**
 * @brief 获取运行标识
 *
 * @return true 正在运行
 * @return false 停止
 */
bool DaemonJson::getRunning() const { return running; }

/**
 * @brief 设置队列空标识
 *
 * @param queue_empty 队列空标识，true 队列空；false 队列非空
 */
void DaemonJson::setQueueEmpty(bool queue_empty) { this->queue_empty = queue_empty; }

/**
 * @brief 获取队列空标识
 *
 * @return true 队列空
 * @return false 队列非空
 */
bool DaemonJson::getQueueEmpty() const { return queue_empty; }

/**
 * @brief 设置队列满标识
 *
 * @param queue_full 队列满标识，true 队列满；false 队列非满
 */
void DaemonJson::setQueueFull(bool queue_full) { this->queue_full = queue_full; }

/**
 * @brief 获取队列满标识
 *
 * @return true 队列满
 * @return false 队列非满
 */
bool DaemonJson::getQueueFull() const { return queue_full; }

/**
 * @brief 设置加载器
 *
 * @param loader 加载器对象
 */
void DaemonJson::setLoader(const LoaderJson &loader) { this->loader = loader; }

/**
 * @brief 获取加载器
 *
 * @return LoaderJson 加载起对象
 */
LoaderJson DaemonJson::getLoader() const { return loader; }

} // namespace daemon
} // namespace hebpf
