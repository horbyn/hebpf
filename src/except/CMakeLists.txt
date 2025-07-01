file(GLOB sources "*.cc")
add_library(hebpf_except
  STATIC
  ${sources}
)
