set(sources acl_rules.cc)
add_library(acl_rules
  ${sources}
)

ebpf_copile(acl.bpf.c)
target_link_libraries(${PROJECT_NAME}_acl
  PRIVATE
  acl_rules
)
