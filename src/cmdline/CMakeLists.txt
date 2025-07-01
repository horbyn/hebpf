file(GLOB sources "*.cc")
add_library(hebpf_cmdline
  STATIC
  ${sources}
)
