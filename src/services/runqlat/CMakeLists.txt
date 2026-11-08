set(sources runqlat_conf.cc)
add_library(runqlat_conf
  ${sources}
)

ebpf_compile(runqlat.bpf.c RUNQLAT_Sched)
target_link_libraries(${PROJECT_NAME}_runqlat_sched
  PRIVATE
  data
  runqlat_conf
)
