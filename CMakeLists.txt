#############
## General ##
#############

cmake_minimum_required (VERSION 3.9)
project (MIPS32 CXX)

#set(CMAKE_MODULE_PATH CMakeModules)

include_directories(include third-party)

set (CMAKE_CXX_STANDARD 17)

set(MIPS32_VERSION_MAJOR 0)
set(MIPS32_VERSION_MINOR 3)
set(MIPS32_VERSION_PATCH 0)

if (NOT (CMAKE_BUILD_TYPE STREQUAL "Coverage"))
    if(${MSVC})
        set(${CMAKE_CXX_FLAGS} "${CMAKE_CXX_FLAGS} /W4 /fp:strict")
        add_definitions("-D_CRT_SECURE_NO_WARNINGS")
    else()
        set(${CMAKE_CXX_FLAGS} "${CMAKE_CXX_FLAGS} -Wall -Wextra -mfpmath=sse -msse2 -pedantic")
    endif()
endif()

#############
## Library ##
#############



###########
## Tests ##
###########

add_executable(Tests
    test/test_main.cpp
    
    src/machine_inspector.cpp

    test/test_ram.cpp
    src/ram.cpp

    #test/test_cache.cpp
    #src/cache.cpp

    test/test_cp1.cpp
    src/cp1.cpp
)

# Catch2 fails on clang5+ due to std::uncaught_exceptions not defined
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(${CMAKE_CXX_FLAGS} "${CMAKE_CXX_FLAGS} -Wno-logical-op-parentheses")
    set_target_properties(Tests PROPERTIES DEFINITIONS "-DCATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    set_target_properties(Tests PROPERTIES COMPILE_FLAGS "-g -O0 --coverage" LINK_FLAGS "-lgcov --coverage")
endif()

include(CTest)
enable_testing()
add_test(NAME AllTests COMMAND Tests)
