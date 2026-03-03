// clang-format off
#include "thread.h"
#include "src/common/exception.h"
// clang-format on

namespace hebpf {
namespace thread {

Thread::~Thread() { join(); }

/**
 * @brief 设置线程名称
 *
 * @param thread_name 名称
 */
void Thread::setName(std::string_view thread_name) { name_ = std::string{thread_name}; }

/**
 * @brief 获取线程名称
 *
 * @return std::string 名称
 */
std::string Thread::getName() const { return name_; }

/**
 * @brief 启动线程
 *
 * @param func 线程入口函数
 */
void Thread::start(std::function<void()> func) {
  thread_ = std::thread([func = std::move(func), name = name_] {
    try {
      if (!name.empty()) {
        pthread_setname_np(pthread_self(), name.c_str());
      }
      func();
    } catch (const except::Exception &exc) {
      GLOBAL_LOG(error, "Thread {} caught exception: {}\nStackfram:\n{}", name, exc.what(),
                 exc.stackFrame());
    } catch (const std::exception &exc) {
      GLOBAL_LOG(error, "Thread {} caught exception: {}", name, exc.what());
    }
  });
}

/**
 * @brief 等待线程结束
 *
 */
void Thread::join() {
  if (joinable()) {
    thread_.join();
  }
}

/**
 * @brief 判断线程是否可 join
 *
 * @return true 可以
 * @return false 不可以
 */
bool Thread::joinable() const { return thread_.joinable(); }

} // namespace thread
} // namespace hebpf
