# 函数 generate_ebpf_ids()
# 功能: 通过 id.map 配置文件生成三个 CMake 变量, 每个变量对应一个头文件
#      这些生成的头文件会被内核态和用户态 eBPF 程序引用
function(generate_ebpf_ids)
  set(MAP_FILE "${CMAKE_SOURCE_DIR}/ebpf_id.map")
  set(IDS_HEADER "${CMAKE_BINARY_DIR}/generated/ebpf_ids.h")
  set(PROG_MAP_HEADER "${CMAKE_BINARY_DIR}/generated/ebpf_prog_map.h")
  set(HOOK_TYPE_HEADER "${CMAKE_BINARY_DIR}/generated/ebpf_hook_type.h")

  get_filename_component(IDS_DIR ${IDS_HEADER} DIRECTORY)
  file(MAKE_DIRECTORY ${IDS_DIR})

  file(READ ${MAP_FILE} FILE_CONTENT)
  string(REPLACE "\n" ";" LINES "${FILE_CONTENT}") # 按换行符分割成列表（兼容 Unix 和 Windows 换行）
  string(REPLACE "\r" "" LINES "${LINES}")         # 处理可能的 \r（Windows 风格）

  set(DEFINES "")
  set(PROG_MAP_ENTRIES "")
  set(PROG_NAME_CONSTANTS "")
  set(HOOK_TYPES "")

  foreach(LINE IN LISTS LINES)
    string(STRIP "${LINE}" LINE_STRIPPED)
    if(LINE_STRIPPED STREQUAL "" OR LINE_STRIPPED MATCHES "^[ \t]*#")
      continue()
    endif()
    if(NOT LINE_STRIPPED MATCHES "[0-9]")
      continue()
    endif()
    string(REGEX MATCH "([^ \t]+)[ \t]+([0-9]+)[ \t]+([^ \t]+)" _ ${LINE_STRIPPED})
    set(NAME ${CMAKE_MATCH_1})
    set(ID ${CMAKE_MATCH_2})
    set(HOOKS_STR ${CMAKE_MATCH_3})
    if(NOT NAME OR NOT ID OR NOT HOOKS_STR)
      message(FATAL_ERROR "Invalid line in ${MAP_FILE}: '${LINE_STRIPPED}'")
    endif()

    set_property(GLOBAL APPEND PROPERTY EBPF_PROG_NAMES ${NAME})
    set_property(GLOBAL PROPERTY EBPF_PROG_ID_${NAME} ${ID})
    string(REPLACE "," ";" HOOK_LIST ${HOOKS_STR})

    set(HOOK_LIST_STR "")
    foreach(hook IN LISTS HOOK_LIST)
      string(STRIP "${hook}" hook_stripped)
      if(NOT hook_stripped STREQUAL "")
        list(APPEND HOOK_TYPES ${hook_stripped})
        if(HOOK_LIST_STR STREQUAL "")
          set(HOOK_LIST_STR "daemon::HookType::${hook_stripped}")
        else()
          set(HOOK_LIST_STR "${HOOK_LIST_STR}, daemon::HookType::${hook_stripped}")
        endif()
      endif()
    endforeach()

    set(DEFINES "${DEFINES}#define EBPF_ID_${NAME} ${ID}\n")
    set(PROG_NAME_CONSTANTS "${PROG_NAME_CONSTANTS}constexpr std::string_view ${NAME}{\"${NAME}\"};\n")
    set(PROG_MAP_ENTRIES "${PROG_MAP_ENTRIES}    { std::string{${NAME}}, { ${ID},  ${HOOK_LIST_STR} } },\n")
  endforeach()

  list(REMOVE_DUPLICATES HOOK_TYPES)
  set(ENUM_DEFINES "")
  foreach(hook ${HOOK_TYPES})
    set(ENUM_DEFINES "${ENUM_DEFINES}  ${hook},\n")
  endforeach()

  configure_file(${CMAKE_SOURCE_DIR}/cmake/ebpf_hook_type.h.in ${HOOK_TYPE_HEADER} @ONLY)
  configure_file(${CMAKE_SOURCE_DIR}/cmake/ebpf_ids.h.in ${IDS_HEADER} @ONLY)
  configure_file(${CMAKE_SOURCE_DIR}/cmake/ebpf_prog_map.h.in ${PROG_MAP_HEADER} @ONLY)
endfunction()
