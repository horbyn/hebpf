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
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/libs/libbpf-bootstrap/tools/cmake)

# 安装外部依赖
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/libs/libbpf-bootstrap)
  message(FATAL_ERROR "请先执行 git submodule update --init --recursive")
endif()
add_subdirectory(libs/libbpf-bootstrap/examples/c)
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
set(LIBBPF_BOOTSTRAP_SOURCE_DIR libs/libbpf-bootstrap)
set(LIBBPF_BOOTSTRAP_BINARY_DIR ${CMAKE_BINARY_DIR}/libs/libbpf-bootstrap/examples/c)
set(BPFOBJECT_BPFTOOL_EXE ${LIBBPF_BOOTSTRAP_BINARY_DIR}/bpftool/bootstrap/bpftool)
set(BPFOBJECT_VMLINUX_H ${CMAKE_SOURCE_DIR}/${LIBBPF_BOOTSTRAP_SOURCE_DIR}/vmlinux.h/include/${ARCH}/vmlinux.h)
set(LIBBPF_INCLUDE_DIRS ${LIBBPF_BOOTSTRAP_BINARY_DIR}/libbpf)
set(LIBBPF_LIBRARIES ${LIBBPF_BOOTSTRAP_BINARY_DIR}/libbpf/libbpf.a)

include(FetchContent)
FetchContent_Declare(
  inotify-cpp
  GIT_REPOSITORY https://github.com/erikzenker/inotify-cpp.git
  GIT_TAG        v1.0.0
)
set(BUILD_EXAMPLE OFF CACHE BOOL "Disable examples")
set(BUILD_TEST OFF CACHE BOOL "Disable tests")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Disable shared libs")
set(BUILD_STATIC_LIBS ON CACHE BOOL "Enable static libs")
FetchContent_MakeAvailable(inotify-cpp)

# 设置全局变量

# 查找第三方库

# 编译
add_subdirectory(src)

# 可执行文件
find_package(
        Boost 1.54.0
        COMPONENTS unit_test_framework system
        REQUIRED
)
find_package(Threads)

add_executable(main main.cc)
target_link_libraries(main
        PRIVATE
        inotify-cpp::inotify-cpp
        ${CMAKE_THREAD_LIBS_INIT})
