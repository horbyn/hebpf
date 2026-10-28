set(sources acl_rules.cc)
add_library(acl_rules
  ${sources}
)

ebpf_compile(acl.bpf.c ACL_TC)
target_link_libraries(${PROJECT_NAME}_acl_tc
  PRIVATE
  acl_rules
)

ebpf_compile(acl_xdp.bpf.c ACL_XDP_Generic)
target_link_libraries(${PROJECT_NAME}_acl_xdp_generic
  PRIVATE
  acl_rules
)
