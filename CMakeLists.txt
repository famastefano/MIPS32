#############
## General ##
#############

cmake_minimum_required (VERSION 3.9)
project (MIPS32 CXX)

set(CMAKE_MODULE_PATH CMakeModules)

include_directories(include third-party)

set (CMAKE_CXX_STANDARD 17)

set(MIPS32_VERSION_MAJOR 0)
set(MIPS32_VERSION_MINOR 3)
set(MIPS32_VERSION_PATCH 0)

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    include(CodeCoverage)
    setup_target_for_coverage(${PROJECT_NAME}_coverage Tests coverage)

    SET(CMAKE_CXX_FLAGS "-g -O0 --coverage")
    set(LCOV_REMOVE_EXTRA "'test/*'" "'third-party/*'")
else()
    if(${MSVC})
        set(${CMAKE_CXX_FLAGS} "${CMAKE_CXX_FLAGS} /W4 /fp:strict")
        add_definitions("-D_CRT_SECURE_NO_WARNINGS")
    else()
        set(${CMAKE_CXX_FLAGS} "${CMAKE_CXX_FLAGS} -Wall -Wextra -mfpmath=sse -msse2")
    endif()
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
add_executable(Tests test/test_main.cpp
    src/machine_inspector.cpp

    test/test_ram.cpp
    src/ram.cpp

    #test/test_cache.cpp
    #src/cache.cpp

    test/test_cp1.cpp
    src/cp1.cpp
)

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    target_link_libraries(Tests INTERFACE "-lgcov --coverage")
endif()

include(CTest)
enable_testing()
add_test(NAME AllTests COMMAND Tests)
