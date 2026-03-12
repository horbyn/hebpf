file(GLOB sources *.cc)
add_library(cmdline
  ${sources}
)
target_link_libraries(cmdline
  PRIVATE
  CLI11::CLI11
)
