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

# 设置全局变量

# 查找第三方库

# 编译
include_directories(${spdlog_SOURCE_DIR}/include)
add_subdirectory(src)

# 可执行文件
add_executable(hebpf hebpf.cc)
target_link_libraries(hebpf
  PRIVATE
  hebpf_log
)
