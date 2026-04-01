set(sources acl_rules.cc)
add_library(acl_rules
  ${sources}
)

ebpf_compile(acl.bpf.c)
target_link_libraries(${PROJECT_NAME}_acl
  PRIVATE
  acl_rules
)
ebpf_compile(acl_xdp.bpf.c)
target_link_libraries(${PROJECT_NAME}_acl_xdp
  PRIVATE
  acl_rules
)
