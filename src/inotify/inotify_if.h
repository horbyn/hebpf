#pragma once

// clang-format off
#include <functional>
#include <string>
#include <string_view>
// clang-format on

namespace hebpf {
namespace inotify {

class InotifyIf {
public:
  using InotifyCb = std::function<void(std::string_view name, uint32_t mask)>;

  virtual ~InotifyIf() = default;

  virtual bool addWatch(std::string_view path, uint32_t mask) = 0;
  virtual bool removeWatch(std::string_view path) = 0;
  virtual void setInotifyCb(InotifyCb cb) = 0;
  virtual void processInotify(const char *buffer, size_t length) = 0;
};

} // namespace inotify
} // namespace hebpf
