set(sources acl_rules.cc)
add_library(acl_rules
  ${sources}
)

ebpf_compile(acl.bpf.c ACL_TC TC_In)
target_link_libraries(${PROJECT_NAME}_acl
  PRIVATE
  acl_rules
)

ebpf_compile(acl_xdp.bpf.c ACL_XDP_GENERIC XDP_Gen)
target_link_libraries(${PROJECT_NAME}_acl_xdp
  PRIVATE
  acl_rules
)
