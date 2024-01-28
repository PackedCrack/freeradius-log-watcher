cmake_minimum_required (VERSION 3.10)

# Setup GoogleTest
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()


set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)


add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)


project (${TEST_PROJECT})

add_executable(${TEST_PROJECT}
        test.cpp
)



set_property(TARGET ${TEST_PROJECT} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${TEST_PROJECT} PROPERTY CXX_STANDARD_REQUIRED ON)

target_include_directories(${TEST_PROJECT} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/googletest-src/googletest/include)
target_link_libraries(${TEST_PROJECT} gtest_main)
use_pch(${TEST_PROJECT})

enable_testing()
add_test(NAME ${TEST_PROJECT} COMMAND ${TEST_PROJECT})