#include "src/log/logger.h"

int main() {
  using namespace hebpf;

  class Loader : log::Loggable<log::Id::loader> {
  public:
    void log() {
      HEBPF_MODULE_LOG(debug, "Hello world!");
    }
  };
  Loader l{};
  l.log();

  HEBPF_LOG(info, "Hello world!");

  return 0;
}
