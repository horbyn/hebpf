set(sources lb_rules.cc)
add_library(lb_rules
  ${sources}
)

ebpf_compile(lb_xdp.bpf.c LB_XDP_Generic)
target_link_libraries(${PROJECT_NAME}_lb_xdp_generic
  PRIVATE
  lb_rules
)
