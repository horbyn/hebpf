cmake_minimum_required(VERSION 3.19)
project("hebpf"
  VERSION 0.1.0
  LANGUAGES C CXX
)
message(STATUS "[版本]: ${PROJECT_NAME}_v${PROJECT_VERSION}")

# 设置选项
option(ENABLE_STATIC_LIBS "使用静态链接" ON)
option(ENABLE_UTESTS "启用单元测试" OFF)

# 设置全局变量
set(EXPORT_TEST_TARGET run_coverage)
set(DEPS_DIR ${CMAKE_SOURCE_DIR}/deps)
set(DEPS_BUILD_DIR ${CMAKE_BINARY_DIR}/deps)
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
  set(ARCH "x86")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
  set(ARCH "arm")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
  set(ARCH "arm64")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc64le")
  set(ARCH "powerpc")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "mips")
  set(ARCH "mips")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "riscv64")
  set(ARCH "riscv")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "loongarch64")
  set(ARCH "loongarch")
endif()

# 编译选项
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  message(STATUS "[编译选项]: debug")
  add_compile_options(-Wall -Wextra -O0 -g3 -D_GLIBCXX_DEBUG)
else()
  message(STATUS "[编译选项]: release")
  add_compile_options(-Wall -Wextra -O2 -DNDEBUG)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置版本
configure_file(
  ${CMAKE_SOURCE_DIR}/hebpf_version.h.in
  ${CMAKE_BINARY_DIR}/hebpf_version.h
  @ONLY
)

# 安装第三方项目
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
FetchContent_Declare(
  libbpf-bootstrap
  GIT_REPOSITORY https://github.com/libbpf/libbpf-bootstrap.git
  GIT_TAG b2b9960d32eb0e99af8e90ceae4b46b96093acb1
  SOURCE_DIR   "${DEPS_DIR}/libbpf-bootstrap_src"
  SUBBUILD_DIR "${DEPS_DIR}/libbpf-bootstrap_subbuild"
)
FetchContent_MakeAvailable(libbpf-bootstrap)

list(APPEND CMAKE_MODULE_PATH ${DEPS_DIR}/libbpf-bootstrap_src/tools/cmake)
set(LIBBPF_BOOTSTRAP_PATH ${DEPS_DIR}/libbpf-bootstrap_src)

include(ExternalProject)
ExternalProject_Add(libbpf
  PREFIX libbpf
  SOURCE_DIR ${LIBBPF_BOOTSTRAP_PATH}/libbpf/src
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make
    BUILD_STATIC_ONLY=1
    OBJDIR=${CMAKE_BINARY_DIR}/libbpf/libbpf
    DESTDIR=${CMAKE_BINARY_DIR}/libbpf
    INCLUDEDIR=
    LIBDIR=
    UAPIDIR=
    install install_uapi_headers
  BUILD_IN_SOURCE TRUE
  # BUILD_ALWAYS 1 实际上有一个 bug 会导致编译的输出中，同样的文件会输出多次，参考：
  # https://gitlab.kitware.com/cmake/cmake/-/issues/22241 有两个解决办法：
  # （1）cmake_minimum_required(VERSION 3.19)
  # （2）cmake_policy(SET CMP0114 NEW)
  BUILD_ALWAYS 1
  INSTALL_COMMAND ""
  STEP_TARGETS build
)

ExternalProject_Add(bpftool
  PREFIX bpftool
  SOURCE_DIR ${LIBBPF_BOOTSTRAP_PATH}/bpftool/src
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make bootstrap
    OUTPUT=${CMAKE_BINARY_DIR}/bpftool/
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS 1
  INSTALL_COMMAND ""
  STEP_TARGETS build
  DEPENDS libbpf-build
)

set(BPFOBJECT_BPFTOOL_EXE ${CMAKE_BINARY_DIR}/bpftool/bootstrap/bpftool)
set(BPFOBJECT_VMLINUX_H ${LIBBPF_BOOTSTRAP_PATH}/vmlinux.h/include/${ARCH}/vmlinux.h)
set(LIBBPF_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/libbpf)
set(LIBBPF_LIBRARIES ${CMAKE_BINARY_DIR}/libbpf/libbpf.a)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.15.1
  SOURCE_DIR   "${DEPS_DIR}/spdlog_src"
  BINARY_DIR   "${DEPS_BUILD_DIR}/spdlog"
  SUBBUILD_DIR "${DEPS_DIR}/spdlog_subbuild"
)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE) # 不要安装到系统目录
FetchContent_MakeAvailable(spdlog)
message(STATUS "spdlog 头文件目录: ${spdlog_SOURCE_DIR}/include")
include_directories(${spdlog_SOURCE_DIR}/include)

FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG v0.9.7
  SOURCE_DIR   "${DEPS_DIR}/magic_enum_src"
  BINARY_DIR   "${DEPS_BUILD_DIR}/magic_enum"
  SUBBUILD_DIR "${DEPS_DIR}/magic_enum_subbuild"
)
FetchContent_MakeAvailable(magic_enum)
message(STATUS "magic_enum 头文件目录: ${magic_enum_SOURCE_DIR}/include")
include_directories(${magic_enum_SOURCE_DIR}/include)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0
  SOURCE_DIR   "${DEPS_DIR}/googletest_src"
  BINARY_DIR   "${DEPS_BUILD_DIR}/googletest"
  SUBBUILD_DIR "${DEPS_DIR}/googletest_subbuild"
)
FetchContent_MakeAvailable(googletest)
message(STATUS "googletest 头文件目录: ${googletest_SOURCE_DIR}/googletest/include")
message(STATUS "googlemock 头文件目录: ${googletest_SOURCE_DIR}/googlemock/include")
include_directories(
  ${googletest_SOURCE_DIR}/googletest/include
  ${googletest_SOURCE_DIR}/googlemock/include
)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG yaml-cpp-0.9.0
  SOURCE_DIR   "${DEPS_DIR}/yaml-cpp_src"
  BINARY_DIR   "${DEPS_BUILD_DIR}/yaml-cpp"
  SUBBUILD_DIR "${DEPS_DIR}/yaml-cpp_subbuild"
)
FetchContent_MakeAvailable(yaml-cpp)
message(STATUS "yaml-cpp 头文件目录: ${yaml-cpp_SOURCE_DIR}/include")
include_directories(${yaml-cpp_SOURCE_DIR}/include)

# 引入头文件
include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/src/services
  ${LIBBPF_INCLUDE_DIRS}
)

find_package(BpfObject REQUIRED)
if(ENABLE_STATIC_LIBS)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(BUILD_SHARED_LIBS OFF)
else()
  set(BUILD_SHARED_LIBS ON)
endif()

# 单元测试
if(ENABLE_UTESTS)
  message(STATUS "[单元测试]: ON")
  find_program(GCOVR_EXE "gcovr")
  if(NOT GCOVR_EXE)
    message(FATAL_ERROR "单元测试依赖: gcovr 不存在\n"
      "Debian/Ubuntu 请使用: apt install -y gcovr 安装"
    )
  endif()

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

  add_custom_target(testclean
    COMMAND ${CMAKE_COMMAND} -E remove -f
      ${CMAKE_BINARY_DIR}/**/*.gcda
    COMMENT "清理前次测试残留数据"
  )

  add_subdirectory(test)
endif()

# 编译项目
add_compile_options(-Werror)
add_subdirectory(src)

add_library(${PROJECT_NAME}_core INTERFACE)
target_link_libraries(${PROJECT_NAME}_core INTERFACE
  common
  daemon
  log
  hello
)
add_library(${PROJECT_NAME}::core ALIAS ${PROJECT_NAME}_core)

add_executable(${PROJECT_NAME} main.cc)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
  ${PROJECT_NAME}::core
)
target_link_options(${PROJECT_NAME}
  PRIVATE
  -rdynamic
)

file(RELATIVE_PATH RELATIVE_BINARY_DIR ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
message(STATUS "需要生成单元测试覆盖率报告, 执行\n"
  "\tcmake --build ${RELATIVE_BINARY_DIR} --target ${EXPORT_TEST_TARGET} -j \$(getconf _NPROCESSORS_ONLN)\n"
  "\t输出目录为 ${CMAKE_BINARY_DIR}/${EXPORT_TEST_TARGET}\n"
)
