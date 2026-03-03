file(GLOB_RECURSE TEST_FILES 
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/test/*test.cc
)
foreach(test_file ${TEST_FILES})
  get_filename_component(test_name ${test_file} NAME_WE)
  list(APPEND TEST_EXE ${test_name})
endforeach()
message(STATUS "单元测试项目: ${TEST_EXE}")

setup_target_for_coverage_gcovr_html(
  NAME ${EXPORT_TEST_TARGET}
  EXECUTABLE cd ${CMAKE_BINARY_DIR}/test && ctest -L Unittests && cd ${CMAKE_BINARY_DIR}
  DEPENDENCIES
    testclean
    ${TEST_EXE}
  BASE_DIRECTORY ${CMAKE_SOURCE_DIR}/src
)

enable_testing()
include(GoogleTest)

# 单元测试宏定义
macro(compile_unit_test target_name)
  set(multiValueArgs LIBS)
  cmake_parse_arguments(UT "" "" "${multiValueArgs}" ${ARGN})

  add_executable(
    ${target_name}
    ${target_name}.cc
  )
  target_link_libraries(${target_name}
    PRIVATE
    GTest::gtest_main
    GTest::gmock_main
    ${PROJECT_NAME}::core
    ${UT_LIBS}
    ${ELF_LIBRARY}
  )
  gtest_discover_tests(${target_name}
    PROPERTIES LABELS "Unittests"
  )
endmacro()

add_subdirectory(services)
