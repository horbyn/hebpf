#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <vector>
#include "src/interface/ebpf_if.h"
// clang-format on

namespace hebpf {
namespace daemon {

class LoaderIf {
public:
  virtual ~LoaderIf() = default;
  virtual bool loadService(std::string_view so_path) = 0;
  virtual bool unloadServices(std::string_view so_path) = 0;
  virtual EbpfIf *getService(std::string_view so_path) const = 0;
  virtual std::vector<std::string> getAllService() const = 0;
};

} // namespace daemon
} // namespace hebpf
