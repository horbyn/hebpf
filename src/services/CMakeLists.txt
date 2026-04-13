macro(ebpf_compile source)
  get_filename_component(source_file ${source} NAME_WE)

  set(skel_file "${CMAKE_CURRENT_BINARY_DIR}/${source_file}.skel.h")
  if(NOT EXISTS ${skel_file})
    file(WRITE ${skel_file} "// Placeholder\n")
  endif()
  set_source_files_properties(${skel_file} PROPERTIES GENERATED TRUE)

  bpf_object(${source_file} ${source_file}.bpf.c)
  add_dependencies(${source_file}_skel libbpf-build bpftool-build)

  set(LIB_NAME ${PROJECT_NAME}_${source_file})
  add_library(${LIB_NAME}
    SHARED # hebpf 项目的动态加载机制需要将 eBPF 程序编译为动态库
    ${source_file}.cc
  )
  set_target_properties(${LIB_NAME}
    PROPERTIES
    POSITION_INDEPENDENT_CODE # 动态库需要与位置无关即编译加上 PIC
    ON
  )
  add_dependencies(${LIB_NAME} ${source_file}_skel)
  target_link_libraries(${LIB_NAME}
    PRIVATE
    ${source_file}_skel
    ${PROJECT_NAME}::core
  )
endmacro()

add_subdirectory(acl)
add_subdirectory(examples)
add_subdirectory(klog)
