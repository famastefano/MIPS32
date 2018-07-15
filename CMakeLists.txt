#############
## General ##
#############

cmake_minimum_required (VERSION 3.9)
project (MIPS32 CXX)

include_directories(include third-party)

set (CMAKE_CXX_STANDARD 17)

set(MIPS32_VERSION_MAJOR 0)
set(MIPS32_VERSION_MINOR 1)
set(MIPS32_VERSION_PATCH 0)

if(${MSVC})
    set(${CMAKE_CXX_FLAGS} "${CMAKE_CXX_FLAGS} /W4 /fp:strict")
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
else()
    set(${CMAKE_CXX_FLAGS} "${CMAKE_CXX_FLAGS} -Wall -Wextra -mfpmath=sse -msse2")
endif()

if(COVERAGE)
    set(${CMAKE_CXX_FLAGS} "${CMAKE_CXX_FLAGS} --coverage")
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

## RAM ##
add_executable(TestRAM test/test_ram.cpp src/machine_inspector.cpp src/ram.cpp)
target_include_directories(TestRAM PRIVATE src)

#add_executable(TestCACHE test/test_cache.cpp src/machine_inspector.cpp src/cache.cpp)
#target_include_directories(TestCACHE PRIVATE src)

add_executable(TestCP1 test/test_cp1.cpp src/machine_inspector.cpp src/cp1.cpp)
target_include_directories(TestCP1 PRIVATE src)

if(COVERAGE)
    target_link_libraries(TestRAM INTERFACE "-lgcov --coverage")
    #target_link_libraries(TestCACHE INTERFACE "-lgcov --coverage")
    target_link_libraries(TestCP1 INTERFACE "-lgcov --coverage")
endif()

include(CTest)
enable_testing()
add_test(NAME TestRam COMMAND TestRAM)
#add_test(NAME TestCache COMMAND TestCACHE)
add_test(NAME TestCp1 COMMAND TestCP1)
