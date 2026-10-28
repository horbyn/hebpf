ebpf_compile(scheduler_tc.bpf.c SCHEDULER_TC)
target_link_libraries(${PROJECT_NAME}_scheduler_tc
  PRIVATE
  daemon
)

ebpf_compile(scheduler_xdp.bpf.c SCHEDULER_XDP_Generic)
target_link_libraries(${PROJECT_NAME}_scheduler_xdp_generic
  PRIVATE
  daemon
)

ebpf_compile(scheduler_sched_switch.bpf.c SCHEDULER_Sched_Switch)
