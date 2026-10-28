set(sources runqlat_conf.cc)
add_library(runqlat_conf
  ${sources}
)

ebpf_compile(runqlat.bpf.c RUNQLAT_Sched_Switch)
target_link_libraries(${PROJECT_NAME}_runqlat_sched_switch
  PRIVATE
  data
  runqlat_conf
)
