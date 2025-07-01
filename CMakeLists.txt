cmake_minimum_required(VERSION 3.17)
project("hebpf" VERSION 0.1.0)
message(STATUS "[版本]: ${PROJECT_NAME}_v${PROJECT_VERSION}")

# 设置全局变量
set(HEBPF_EXPORT_TEST_TARGET run_coverage)
set(HEBPF_EXPORT_BENCHMARK_TARGET run_benchmark)
set(HEBPF_EXPORT_STATIC_ANALYSIS_TARGET run_static_analysis)
set(HEBPF_EXPORT_MEMCHECK_TARGET run_memcheck)
set(HEBPF_EXPORT_FLAMEGRAPH_TARGET run_flamegraph)

# 编译选项
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  message(STATUS "[编译选项]: debug")
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

# 设置版本
configure_file(
  ${CMAKE_SOURCE_DIR}/hebpf_version.h.in
  ${CMAKE_BINARY_DIR}/hebpf_version.h
  @ONLY
)

# 安装外部依赖
set(HEBPF_DEPS_DIR ${CMAKE_SOURCE_DIR}/deps)
include(FetchContent)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.15.1
  SOURCE_DIR   "${HEBPF_DEPS_DIR}/spdlog_src"
  BINARY_DIR   "${HEBPF_DEPS_DIR}/spdlog_build"
  SUBBUILD_DIR "${HEBPF_DEPS_DIR}/spdlog_subbuild"
)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE) # 不要安装到系统目录
FetchContent_MakeAvailable(spdlog)
message(STATUS "spdlog 头文件目录: ${spdlog_SOURCE_DIR}/include")

FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG v2.5.0
  SOURCE_DIR   "${HEBPF_DEPS_DIR}/cli11_src"
  BINARY_DIR   "${HEBPF_DEPS_DIR}/cli11_build"
  SUBBUILD_DIR "${HEBPF_DEPS_DIR}/cli11_subbuild"
)
FetchContent_MakeAvailable(cli11)
message(STATUS "cli11 头文件目录: ${cli11_SOURCE_DIR}/include")

# 引入头文件
include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${spdlog_SOURCE_DIR}/include
  ${cli11_SOURCE_DIR}/include
)

# 单元测试
option(HEBPF_TEST "Enable unit test" OFF)
if(HEBPF_TEST)
  # 下载代码覆盖率统计 cmake 模块
  set(download_file_url "https://raw.githubusercontent.com/bilke/cmake-modules/master/CodeCoverage.cmake")
  set(download_file_path "${CMAKE_SOURCE_DIR}/cmake/CodeCoverage.cmake")

  if(NOT EXISTS "${download_file_path}")
    file(DOWNLOAD ${download_file_url} ${download_file_path} SHOW_PROGRESS)
  endif()

  include(CheckCCompilerFlag)
  include(${CMAKE_SOURCE_DIR}/cmake/CodeCoverage.cmake)
  add_compile_definitions(Debug)
  APPEND_COVERAGE_COMPILER_FLAGS()

  add_subdirectory(test)
endif()

# 基准测试
option(HEBPF_BENCHMARK "Enable benchmark" OFF)
if(HEBPF_BENCHMARK)
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
  add_subdirectory(benchmarks)
endif()

# 编译
add_subdirectory(src)

# 接口库
add_library(${PROJECT_NAME}_core INTERFACE)
target_link_libraries(${PROJECT_NAME}_core INTERFACE
  hebpf_log
  hebpf_cmdline
  hebpf_except
)
add_library(${PROJECT_NAME}::core ALIAS ${PROJECT_NAME}_core)

# 可执行文件
add_executable(${PROJECT_NAME} hebpf.cc)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
  ${PROJECT_NAME}::core
)

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

  add_custom_target(${HEBPF_EXPORT_STATIC_ANALYSIS_TARGET}
    COMMAND ${CLANG_TIDY_EXE}
      -p ${CMAKE_BINARY_DIR}/compile_commands.json
      --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
      ${ALL_SROUCES}
    COMMENT "对项目进行静态检查"
  )
endif()

# 内存检查
option(HEBPF_MEMCHECK "Enable memory check" OFF)
if(HEBPF_MEMCHECK)
  message(STATUS "[内存检查]: ON")

  find_program(VALGRIND_EXE "valgrind")
  if (NOT VALGRIND_EXE)
    message(FATAL_ERROR "找不到 valgrind 工具")
  endif()

  add_custom_target(${HEBPF_EXPORT_MEMCHECK_TARGET}
    COMMAND ${VALGRIND_EXE}
      --leak-check=full
      --track-origins=yes
      --leak-check=full
      --show-leak-kinds=all
      ${CMAKE_BINARY_DIR}/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    COMMENT "对项目进行内存检查"
  )
endif()

# 火焰图
option(HEBPF_FLAMEGRAPH "Enable flamegraph" OFF)
if(HEBPF_FLAMEGRAPH)
  message(STATUS "[火焰图集成]: ON")

  execute_process(
    COMMAND perf --version
    RESULT_VARIABLE PERF_RESULT
    OUTPUT_VARIABLE PERF_OUTPUT
    ERROR_VARIABLE PERF_ERR
  )
  if (PERF_RESULT EQUAL 0)
    string(REGEX MATCH "^perf version [0-9]+\\.[0-9]+\\.[0-9]+" PERF_VERSION ${PERF_OUTPUT})
    if (NOT PERF_VERSION)
      message(FATAL_ERROR "perf 命令损坏: ${PERF_OUTPUT}")
    endif()
  else()
    message(FATAL_ERROR "找不到 perf 命令: ${PERF_ERR}")
  endif()

  FetchContent_Declare(
    flamegraph
    GIT_REPOSITORY https://github.com/brendangregg/FlameGraph.git
    GIT_TAG v1.0
    SOURCE_DIR   "${HEBPF_DEPS_DIR}/flamegraph_src"
    BINARY_DIR   "${HEBPF_DEPS_DIR}/flamegraph_build"
    SUBBUILD_DIR "${HEBPF_DEPS_DIR}/flamegraph_subbuild"
  )
  FetchContent_MakeAvailable(flamegraph)

  set(HEBPF_FG_DIR ${CMAKE_BINARY_DIR}/${HEBPF_EXPORT_FLAMEGRAPH_TARGET})
  if (NOT EXISTS ${HEBPF_FG_DIR})
    file(MAKE_DIRECTORY ${HEBPF_FG_DIR})
  endif()
  add_custom_target(${HEBPF_EXPORT_FLAMEGRAPH_TARGET} ALL
    COMMAND perf record -g ${CMAKE_BINARY_DIR}/${PROJECT_NAME}
    COMMAND perf script -i perf.data > perf.unfold
    COMMAND ${flamegraph_SOURCE_DIR}/stackcollapse-perf.pl perf.unfold > perf.folded
    COMMAND ${flamegraph_SOURCE_DIR}/flamegraph.pl perf.folded > ${PROJECT_NAME}.svg
    COMMAND ${CMAKE_COMMAND} -E echo "已生成火焰图: ${HEBPF_FG_DIR}/${PROJECT_NAME}.svg"
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${HEBPF_FG_DIR}
    COMMENT "生成火焰图"
  )
endif()

message(STATUS "需要生成单元测试覆盖率报告，运行\n"
  "\tcmake --build <BUILD_DIR> --target ${HEBPF_EXPORT_TEST_TARGET} -j <CORES>\n"
  "\t输出目录为 ${CMAKE_BINARY_DIR}/${HEBPF_EXPORT_TEST_TARGET}"
)
message(STATUS "需要生成基准测试报告，运行\n"
  "\tcmake --build <BUILD_DIR> --target ${HEBPF_EXPORT_BENCHMARK_TARGET} -j <CORES>\n"
  "\t输出目录为 ${CMAKE_BINARY_DIR}/${HEBPF_EXPORT_BENCHMARK_TARGET}"
)
message(STATUS "需要进行代码静态分析，运行\n"
  "\tcmake --build <BUILD_DIR> --target ${HEBPF_EXPORT_STATIC_ANALYSIS_TARGET}"
)
message(STATUS "需要进行检查内存使用情况，运行\n"
  "\tcmake --build <BUILD_DIR> --target ${HEBPF_EXPORT_MEMCHECK_TARGET} -j <CORES>"
)
message(STATUS "需要生成火焰图，运行\n"
  "\tcmake --build <BUILD_DIR> --target ${HEBPF_EXPORT_FLAMEGRAPH_TARGET} -j <CORES>\n"
  "\t输出目录为 ${CMAKE_BINARY_DIR}/${HEBPF_EXPORT_FLAMEGRAPH_TARGET}"
)
