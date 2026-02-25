// clang-format off
#include "inotify_fd.h"
#include <sys/inotify.h>
#include "src/common/exception.h"
// clang-format on

namespace hebpf {
namespace inotify {

InotifyFd::InotifyFd() : Fd{inotify_init1(IN_NONBLOCK)} {
  if (fd() == FD_INVALID) {
    throw EXCEPT("inotify_init1() failed", true);
  }
}

} // namespace inotify
} // namespace hebpf
