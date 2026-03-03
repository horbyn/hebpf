file(GLOB sources *.cc)
add_library(ebpf
  ${sources}
)
add_dependencies(ebpf
  libbpf-build # 这个 target 是从 libbpf-bootstrap 项目中导出的
)
target_link_libraries(ebpf
  ${LIBBPF_LIBRARIES}
  ${ELF_LIBRARY}
)
