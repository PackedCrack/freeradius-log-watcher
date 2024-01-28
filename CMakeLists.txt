cmake_minimum_required(VERSION 3.20)

SET(OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

SET(MAIN_PROJECT freerad-log-watcher)
SET(TEST_PROJECT TESTS)
project(${MAIN_PROJECT} LANGUAGES C CXX)


include(cmake/CompilerWarnings.cmake)
include(cmake/pch.cmake)
include(cmake/cppcheck.cmake)



add_executable(${MAIN_PROJECT})
#add_dependencies(${MAIN_PROJECT} CPPCHECK)
add_subdirectory("src")
# properties
set_target_properties(${MAIN_PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
set_target_properties(${MAIN_PROJECT} PROPERTIES CXX_STANDARD 23)
set_target_properties(${MAIN_PROJECT} PROPERTIES C_STANDARD 11)
set_target_properties(${MAIN_PROJECT} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${MAIN_PROJECT} PROPERTIES C_STANDARD_REQUIRED ON)
# compiler settings etc
apply_compiler_warnings(${MAIN_PROJECT})
use_pch(${MAIN_PROJECT})


enable_testing()
add_subdirectory("test")
# include dirs
target_include_directories(${TEST_PROJECT} PRIVATE "${PROJECT_SOURCE_DIR}/external/include")
target_include_directories(${TEST_PROJECT} PRIVATE "${PROJECT_SOURCE_DIR}/src")