file(GLOB sources *.cc)

# 要共享的 json 对象
list(REMOVE_ITEM sources
  ${CMAKE_CURRENT_SOURCE_DIR}/config_json.cc
)

add_library(daemon
  ${sources}
)
add_library(config_json
  OBJECT
  config_json.cc
)

target_link_libraries(daemon
  PRIVATE
  yaml-cpp::yaml-cpp
  config_json
)
