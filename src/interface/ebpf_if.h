#pragma once

// clang-format off
#include <memory>
#include <string>
#include <string_view>
// clang-format on

#define EXPORT __attribute__((visibility("default")))

constexpr std::string_view SERVICE_CREATE{"create_service"};

namespace hebpf {

class EbpfIf {
public:
  virtual ~EbpfIf() = default;

  virtual std::string getName() const = 0;
  virtual void open() = 0;
  virtual void load() = 0;
  virtual void attach() = 0;
  virtual void detach() = 0;
  virtual void destroy() = 0;
};

} // namespace hebpf

using CreateServiceFn = std::unique_ptr<hebpf::EbpfIf> (*)();

extern "C" {
// 动态库导出的接口必须以 C 风格命名，否则会被 C++ mangle 特性污染
extern std::unique_ptr<hebpf::EbpfIf> create_service();
}
