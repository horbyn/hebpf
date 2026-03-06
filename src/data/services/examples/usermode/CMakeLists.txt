file(GLOB sources *.cc)
add_library(data_usermode
  ${sources}
)
target_link_libraries(data_usermode
  PRIVATE
  data
)
