FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.5
  SOURCE_DIR   "${DEPS_DIR}/benchmark_src"
  BINARY_DIR   "${DEPS_BUILD_DIR}/benchmark_build"
  SUBBUILD_DIR "${DEPS_DIR}/benchmark_subbuild"
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Enable testing of the benchmark library." FORCE)
FetchContent_MakeAvailable(benchmark)
message(STATUS "benchmack 头文件目录: ${benchmark_SOURCE_DIR}/include")
include_directories(${benchmark_SOURCE_DIR}/include)

file(GLOB_RECURSE BM_FILES
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/benchmarks/*bm.cc # 基准测试可执行程序需要以 "_bm" 结尾
)
foreach(bm_file ${BM_FILES})
  get_filename_component(bm_name ${bm_file} NAME_WE)
  list(APPEND BM_EXE ${bm_name})
endforeach()
message(STATUS "基准测试项目: ${BM_EXE}")

add_custom_target(${EXPORT_BENCHMARK_TARGET}
  COMMAND cd ${CMAKE_BINARY_DIR}/benchmarks
  COMMAND ctest -L Benchmarks --verbose
  DEPENDS ${BENCHMARK_EXE}
  COMMENT "运行基准测试"
)

# 基准测试宏定义
macro(benchmark_test target_name)
  file(GLOB sources "*.cc")
  add_executable(
    ${target_name}
    ${sources}
  )
  target_link_libraries(${target_name}
    PRIVATE
    benchmark::benchmark
    GTest::gtest_main
    GTest::gmock_main
    ${PROJECT_NAME}::core
  )

  target_compile_options(${target_name}
    PRIVATE
    -O3     # 启用最大优化
  )

  add_test(
    NAME ${target_name}
    COMMAND ${target_name}
      --benchmark_time_unit=ns
      --benchmark_format=console
  )

  set_property(
    TEST ${target_name}
    APPEND PROPERTY LABELS "Benchmarks"
  )
endmacro()

enable_testing()

add_subdirectory(data)
