file(GLOB sources "*.cc")
add_library(hebpf_log
  STATIC
  ${sources}
)
