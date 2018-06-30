#############
## General ##
#############

cmake_minimum_required (VERSION 3.9)
project (MIPS32 CXX)

set (CMAKE_CXX_STANDARD 17)

set(MIPS32_VERSION_MAJOR 0)
set(MIPS32_VERSION_MINOR 1)
set(MIPS32_VERSION_PATCH 0)

if(${MSVC})
    set(${CMAKE_CXX_FLAGS} "${CMAKE_CXX_FLAGS} /W4")
else()
    set(${CMAKE_CXX_FLAGS} "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

#############
## Library ##
#############

###########
## Tests ##
###########

# Catch2 fails on clang5+ due to std::uncaught_exceptions not defined
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    add_definitions(-DCATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS)
endif()

add_library(Catch2 INTERFACE)
target_include_directories(Catch2 INTERFACE ${CMAKE_SOURCE_DIR}/third-party)

add_executable(my_test ${CMAKE_SOURCE_DIR}/test/test.cpp)
target_link_libraries(my_test Catch2)

include(CTest)
enable_testing()
add_test(NAME MyAwesomeTest COMMAND my_test)
