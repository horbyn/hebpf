file(GLOB sources "*.cc")
add_library(hebpf_log
  STATIC
  ${sources}
)
target_link_libraries(hebpf_log
  PRIVATE
  spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>
)
