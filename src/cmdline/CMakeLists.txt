file(GLOB sources "*.cc")
add_library(hebpf_cmdline
  STATIC
  ${sources}
)
target_link_libraries(hebpf_cmdline
  PRIVATE
  CLI11::CLI11
)
