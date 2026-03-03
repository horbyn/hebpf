#pragma once

// clang-format off
#include <functional>
#include <string>
#include <string_view>
#include "src/callback/callback.h"
// clang-format on

namespace hebpf {
namespace inotify {

using InotifyCb = common::Callback<void(std::string_view name, uint32_t mask)>;

class InotifyIf {
public:
  virtual ~InotifyIf() = default;

  virtual bool addWatch(std::string_view path, uint32_t mask) = 0;
  virtual bool removeWatch(std::string_view path) = 0;
  virtual void setInotifyCb(InotifyCb cb) = 0;
  virtual void processInotify(const char *buffer, size_t length) = 0;
  virtual int getFd() const noexcept = 0;
};

} // namespace inotify
} // namespace hebpf
