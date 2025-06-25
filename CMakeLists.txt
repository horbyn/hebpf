cmake_minimum_required(VERSION 3.10)
project("hebpf" VERSION 0.1.0)
message(STATUS "[版本]: ${PROJECT_NAME}_v${PROJECT_VERSION}")

# 编译选项
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  message(STATUS "[编译选项]: debug")
  add_definitions(-DDEBUG)
else()
  message(STATUS "[编译选项]: release")
  add_compile_options(-O2)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(
  -Wall
  -Wextra
)

# 安装外部依赖
set(HEBPF_DEPS_DIR ${CMAKE_SOURCE_DIR}/deps)
# - git submodule：当需要独立进行版本控制时
# - ExternalProject：不需要控制版本，但需要预编译，且不需要跳入源码
# - FetchContent：不需要控制版本，不需要预编译，但希望能够跳入源码
include(FetchContent)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.15.1
  SOURCE_DIR     "${HEBPF_DEPS_DIR}/spdlog_src"
  BINARY_DIR     "${HEBPF_DEPS_DIR}/spdlog_build"
  SUBBUILD_DIR   "${HEBPF_DEPS_DIR}/spdlog_subbuild"
)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE) # 不要安装到系统目录
FetchContent_MakeAvailable(spdlog)
message(STATUS "spdlog 头文件目录: ${spdlog_SOURCE_DIR}/include")

FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG v2.5.0
  SOURCE_DIR     "${HEBPF_DEPS_DIR}/cli11_src"
  BINARY_DIR     "${HEBPF_DEPS_DIR}/cli11_build"
  SUBBUILD_DIR   "${HEBPF_DEPS_DIR}/cli11_subbuild"
)
FetchContent_MakeAvailable(cli11)
message(STATUS "cli11 头文件目录: ${cli11_SOURCE_DIR}/include")

# 静态检查
option(HEBPF_STATIC_ANALYSIS "Enable static analysis" OFF)
if(HEBPF_STATIC_ANALYSIS)
  message(STATUS "[静态检查]: ON")
  option(CMAKE_EXPORT_COMPILE_COMMANDS "Export compile_commands.json" ON)
  if (NOT CMAKE_EXPORT_COMPILE_COMMANDS)
    message(FATAL_ERROR "需要设置 CMAKE_EXPORT_COMPILE_COMMANDS 以导出"
                        " compile_commands.json")
  endif()

  find_program(CLANG_TIDY_EXE "clang-tidy")
  if(NOT CLANG_TIDY_EXE)
    message(FATAL_ERROR "找不到 clang-tidy 工具")
  endif()

  file(GLOB_RECURSE ALL_SROUCES
    "${CMAKE_SOURCE_DIR}/hebpf.cc"
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.tpp"
    "${CMAKE_SOURCE_DIR}/src/*.cc"
  )

  add_custom_target(static_analysis
    COMMAND ${CLANG_TIDY_EXE}
      -p ${CMAKE_BINARY_DIR}/compile_commands.json
      --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
      ${ALL_SROUCES}
    COMMENT "对项目进行静态检查"
  )
endif()

# 设置全局变量

# 查找第三方库

# 编译
include_directories(
  ${CMAKE_SOURCE_DIR}
  ${spdlog_SOURCE_DIR}/include
  ${cli11_SOURCE_DIR}/include
)
add_subdirectory(src)

# 可执行文件
add_executable(${PROJECT_NAME} hebpf.cc)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
  hebpf_log
  hebpf_cmdline
)

# 内存检查
option(HEBPF_MEMCHECK "Enable memory check" OFF)
if(HEBPF_MEMCHECK)
  message(STATUS "[内存检查]: ON")

  find_program(VALGRIND_EXE "valgrind")
  if (NOT VALGRIND_EXE)
    message(FATAL_ERROR "找不到 valgrind 工具")
  endif()

  add_custom_target(memcheck
    COMMAND ${VALGRIND_EXE}
      --leak-check=full
      --track-origins=yes
      --leak-check=full
      --show-leak-kinds=all
      ${CMAKE_BINARY_DIR}/${PROJECT_NAME}
        --help
    COMMENT "对项目进行内存检查"
  )
  add_dependencies(memcheck ${PROJECT_NAME})
endif()
