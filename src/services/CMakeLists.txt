macro(ebpf_compile source prog_name hook_type)
  get_filename_component(source_file ${source} NAME_WE)

  set(TEMP_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/${source_file}_tmp.bpf.c")
  set(ABS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${source}")

  add_custom_command(
    OUTPUT ${TEMP_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E echo "#include \"generated/ebpf_ids.h\"" > ${TEMP_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E echo "#define THIS_ID EBPF_ID_${prog_name}" >> ${TEMP_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E echo "#define HOOK_TYPE ${hook_type}" >> ${TEMP_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E echo "" >> ${TEMP_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E cat ${ABS_SOURCE} >> ${TEMP_SOURCE}
    VERBATIM
    DEPENDS ${ABS_SOURCE}
    COMMENT "Generating temporary BPF source for ${TEMP_SOURCE}"
  )
  set_source_files_properties(${TEMP_SOURCE} PROPERTIES GENERATED TRUE)

  set(skel_file "${CMAKE_CURRENT_BINARY_DIR}/${source_file}.skel.h")
  if(NOT EXISTS ${skel_file})
    file(WRITE ${skel_file} "// Placeholder\n")
  endif()
  set_source_files_properties(${skel_file} PROPERTIES GENERATED TRUE)

  set(BPF_O_FILE "${CMAKE_CURRENT_BINARY_DIR}/${source_file}.bpf.o")
  add_custom_command(
    OUTPUT ${BPF_O_FILE}
    COMMAND ${BPFOBJECT_CLANG_EXE} -g -O2 -target bpf -D__TARGET_ARCH_${ARCH}
            ${CLANG_SYSTEM_INCLUDES} -I${GENERATED_VMLINUX_DIR}
            -I${CMAKE_SOURCE_DIR} -I${CMAKE_BINARY_DIR}
            -I${CMAKE_CURRENT_SOURCE_DIR}
            -isystem ${LIBBPF_INCLUDE_DIRS}
            -c ${TEMP_SOURCE} -o ${BPF_O_FILE}
    COMMAND_EXPAND_LISTS
    VERBATIM
    DEPENDS ${TEMP_SOURCE} ${GENERATED_VMLINUX_H}
    COMMENT "Compiling eBPF object ${source_file}"
  )
  add_custom_target(${source_file}_bpf ALL DEPENDS ${BPF_O_FILE})

  # 生成 skeleton 头文件
  set(BPF_SKEL_FILE "${CMAKE_CURRENT_BINARY_DIR}/${source_file}.skel.h")
  add_custom_command(
    OUTPUT ${BPF_SKEL_FILE}
    COMMAND bash -c "${BPFOBJECT_BPFTOOL_EXE} gen skeleton ${BPF_O_FILE} > ${BPF_SKEL_FILE}"
    VERBATIM
    DEPENDS ${BPF_O_FILE}
    COMMENT "Generating skeleton for ${source_file}"
  )
  set(OUTPUT_TARGET ${source_file}_skel)
  add_library(${OUTPUT_TARGET} INTERFACE)
  target_sources(${OUTPUT_TARGET} INTERFACE ${BPF_SKEL_FILE})
  target_include_directories(${OUTPUT_TARGET} INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
  target_include_directories(${OUTPUT_TARGET} SYSTEM INTERFACE ${LIBBPF_INCLUDE_DIRS})
  target_link_libraries(${OUTPUT_TARGET} INTERFACE ${LIBBPF_LIBRARIES} -lelf -lz)
  add_dependencies(${OUTPUT_TARGET} libbpf-build bpftool-build)

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
add_subdirectory(lb)
add_subdirectory(scheduler)
