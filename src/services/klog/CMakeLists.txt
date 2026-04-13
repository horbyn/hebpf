set(sources klog.cc)
add_library(klog
  ${sources}
)
target_link_libraries(klog
  PRIVATE
  ebpf
)
