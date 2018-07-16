#############
## General ##
#############

cmake_minimum_required (VERSION 3.9)
project (MIPS32 CXX)

#include_directories(include third-party)

set(MIPS32_VERSION_MAJOR 0)
set(MIPS32_VERSION_MINOR 3)
set(MIPS32_VERSION_PATCH 0)

#############
## Library ##
#############



###########
## Tests ##
###########

add_executable(Tests
    test/test_main.cpp
    
    src/machine_inspector.cpp
# RAM
    test/test_ram.cpp
    src/ram.cpp
# Cache
    #test/test_cache.cpp
    #src/cache.cpp
# Coprocessor 1
    test/test_cp1.cpp
    src/cp1.cpp
)

target_compile_features(Tests PRIVATE cxx_std_17)
target_include_directories(Tests PRIVATE include third-party)

if (NOT (CMAKE_BUILD_TYPE STREQUAL "Coverage"))
    if(MSVC)

        target_compile_options(Tests PRIVATE "/W4")
        target_compile_options(Tests PRIVATE "/fp:strict")
        target_compile_definitions(Tests PRIVATE "-D_CRT_SECURE_NO_WARNINGS")

    else() # GCC - Clang
        
        if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")

            target_compile_options(Tests PRIVATE -Wno-logical-op-parentheses)
            
        else() # GCC
            target_compile_options(Tests PRIVATE -Wparentheses)
        endif()

        target_compile_options(Tests PRIVATE "$<$<STREQUAL:${ARCH},x86>:-m32>")
        target_link_libraries(Tests PRIVATE "$<$<STREQUAL:${ARCH},x86>:-m elf_i386>")
        
        target_compile_options(Tests PRIVATE "$<$<STREQUAL:${ARCH},x64>:-m64>")

        target_compile_options(Tests PRIVATE -Wall -Wextra -mfpmath=sse -msse2 -pedantic)

    endif() # GCC - Clang
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    target_compile_options(Tests PRIVATE -g -O0 --coverage)
    target_link_libraries(Tests PRIVATE -lgcov --coverage)
endif()

include(CTest)
enable_testing()
add_test(NAME AllTests COMMAND Tests)
