#pragma once

// clang-format off
#include <memory>
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"
#include "src/daemon/configurable.h"
#include "src/io/io_if.h"
// clang-format on

#define EXPORT __attribute__((visibility("default")))

constexpr std::string_view SERVICE_CREATE{"create_service"};

namespace hebpf {
namespace ebpf {

class EbpfIf : public daemon::Configurable {
public:
  virtual ~EbpfIf() = default;

  virtual std::string getName() const = 0;
  virtual void open() = 0;
  virtual void load() = 0;
  virtual void attach() = 0;
  virtual void detach() = 0;
  virtual void destroy() = 0;
  virtual bool start(std::weak_ptr<io::IoIf> io_ctx = {}) = 0;
  virtual void stop() = 0;
  virtual nlohmann::json getStatus() const = 0;
};

/**
 * @brief 定义一个 CRTP, 因为所有继承它的 eBPF 程序的 skeleton 都是不一样的类型
 *
 */
template <typename T>
class EbpfSkelIf : public EbpfIf {
public:
  explicit EbpfSkelIf(std::unique_ptr<T> skel = {});

  EbpfSkelIf(const EbpfSkelIf &) = delete;
  EbpfSkelIf &operator=(const EbpfSkelIf &) = delete;
  EbpfSkelIf(EbpfSkelIf &&other) noexcept;
  EbpfSkelIf &operator=(EbpfSkelIf &&other) noexcept;

  operator bool() const;

protected:
  void open() override;
  void load() override;
  void attach() override;
  void detach() override;
  void destroy() override;
  bool start(std::weak_ptr<io::IoIf> io_ctx = {}) override;
  nlohmann::json getStatus() const override;
  void onConfigUpdate(const nlohmann::json &config) override;

  std::unique_ptr<T> skel_;
  std::weak_ptr<io::IoIf> io_ctx_;
};

} // namespace ebpf
} // namespace hebpf

#include "ebpf_if.tpp"

using CreateServiceFn = std::unique_ptr<hebpf::ebpf::EbpfIf> (*)();

extern "C" {
// 动态库导出的接口必须以 C 风格命名，否则会被 C++ mangle 特性污染
extern std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
