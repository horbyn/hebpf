file(GLOB sources *.cc)
add_library(ebpf
  ${sources}
)
target_link_libraries(ebpf
  ${LIBBPF_LIBRARIES}
  ${ELF_LIBRARY}
  ${Z_LIBRARY}
  config_json
  ${PROJECT_NAME}_scheduler_tc
  ${PROJECT_NAME}_scheduler_xdp_generic
  ${PROJECT_NAME}_scheduler_sched
)
