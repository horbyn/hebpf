#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "src/fd/fd_if.h"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace ebpf {

class EbpfNetHookIf : public log::Loggable<log::Id::ebpf> {
public:
  explicit EbpfNetHookIf(int ifindex);
  virtual ~EbpfNetHookIf();

  EbpfNetHookIf(const EbpfNetHookIf &) = delete;
  EbpfNetHookIf &operator=(const EbpfNetHookIf &) = delete;

  int getIfindex() const noexcept;
  bool isAttached() const noexcept;

  bool attach(std::unique_ptr<FdIf> fd);
  bool detach();

protected:
  virtual int do_attach(int prog_fd) = 0;
  virtual int do_detach() = 0;
  virtual int pre_attach_check() = 0;
  virtual void post_detach_cleanup() = 0;

  int ifindex_;
  bool attached_;
  std::unique_ptr<FdIf> prog_fd_;
};

} // namespace ebpf
} // namespace hebpf
