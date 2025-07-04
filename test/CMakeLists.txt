message(STATUS "[单元测试]: ON")
find_program(GCOVR_EXE "gcovr")
if(NOT GCOVR_EXE)
  message(FATAL_ERROR "找不到 gcovr 工具")
endif()

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0
  SOURCE_DIR   "${HEBPF_DEPS_DIR}/googletest_src"
  BINARY_DIR   "${HEBPF_DEPS_DIR}/googletest_build"
  SUBBUILD_DIR "${HEBPF_DEPS_DIR}/googletest_subbuild"
)
FetchContent_MakeAvailable(googletest)
message(STATUS "googletest 头文件目录: ${googletest_SOURCE_DIR}/googletest/include")
message(STATUS "googlemock 头文件目录: ${googletest_SOURCE_DIR}/googlemock/include")
include_directories(
  ${googletest_SOURCE_DIR}/googletest/include
  ${googletest_SOURCE_DIR}/googlemock/include
)

add_custom_target(testclean
  COMMAND ${CMAKE_COMMAND} -E rm -f
    ${CMAKE_BINARY_DIR}/**/*.gcda
  COMMENT "清理前次测试残留数据"
)

# 单元测试可执行程序需要以 "_test" 结尾
file(GLOB DIR RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} */)
foreach(subdir ${DIR})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
    get_filename_component(subdir ${subdir} NAME)
    list(APPEND TEST_EXE "${subdir}_test")
  endif()
endforeach()
message(STATUS "单元测试项目: ${TEST_EXE}")

setup_target_for_coverage_gcovr_html(
  NAME ${HEBPF_EXPORT_TEST_TARGET}
  EXECUTABLE cd ${CMAKE_BINARY_DIR}/test && ctest -L Unittests
  DEPENDENCIES
    testclean
    ${TEST_EXE}
  BASE_DIRECTORY ${CMAKE_SOURCE_DIR}/src
)

enable_testing()
include(GoogleTest)

# 单元测试宏定义
macro(hebpf_unit_test target_name)
  file(GLOB sources "*.cc")
  add_executable(
    ${target_name}
    ${sources}
  )
  target_link_libraries(${target_name}
    PRIVATE
    GTest::gtest_main
    ${PROJECT_NAME}::core
  )
  gtest_discover_tests(${target_name}
    PROPERTIES LABELS "Unittests"
  )
endmacro()

add_subdirectory(cmdline)
