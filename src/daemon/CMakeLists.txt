file(GLOB sources *.cc)
add_library(daemon
  ${sources}
)
target_link_libraries(daemon
  PRIVATE
  yaml-cpp::yaml-cpp
  data_usermode
)
