file(GLOB sources *.cc)
add_library(inotify
  ${sources}
)
target_link_libraries(inotify
  PRIVATE
  fd
)
