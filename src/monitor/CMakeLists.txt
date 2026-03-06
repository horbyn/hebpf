file(GLOB sources *.cc)
add_library(monitor
  ${sources}
)
target_link_libraries(monitor
  PRIVATE
  prometheus-cpp::pull
)
