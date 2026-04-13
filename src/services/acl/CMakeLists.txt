set(sources acl_rules.cc)
add_library(acl_rules
  ${sources}
)

# libbpf-bootstrap 项目只暴露了 CLANG_SYSTEM_INCLUDES 宏定义可以传入头文件路径
set(CLANG_SYSTEM_INCLUDES ${CLANG_SYSTEM_INCLUDES} -I${CMAKE_SOURCE_DIR})

ebpf_compile(acl.bpf.c)
target_link_libraries(${PROJECT_NAME}_acl
  PRIVATE
  acl_rules
)

ebpf_compile(acl_xdp.bpf.c)
target_link_libraries(${PROJECT_NAME}_acl_xdp
  PRIVATE
  acl_rules
)
