cmake_minimum_required (VERSION 3.9)
project (MIPS32 CXX)

set (CMAKE_CXX_STANDARD 17)

set(MIPS32_VERSION_MAJOR 0)
set(MIPS32_VERSION_MINOR 1)
set(MIPS32_VERSION_PATCH 0)

# Tests

add_library(Catch2 INTERFACE)
target_include_directories(Catch2 INTERFACE ${CMAKE_SOURCE_DIR}/third-party/catch.hpp)

add_executable(my_test ${CMAKE_SOURCE_DIR}/test/test.cpp)
target_link_libraries(my_test Catch2)

enable_testing()
add_test(NAME MyAwesomeTest COMMAND my_test)
