file(GLOB sources *.cc)
add_library(data
  ${sources}
)

add_subdirectory(services)
