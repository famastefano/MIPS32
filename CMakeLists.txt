#############
## General ##
#############

cmake_minimum_required (VERSION 3.9)
project (MIPS32 CXX)

#include_directories(include third-party)

set(MIPS32_VERSION_MAJOR 0)
set(MIPS32_VERSION_MINOR 5)
set(MIPS32_VERSION_PATCH 0)

#############
## Library ##
#############

add_library(fs-mips32 SHARED
    src/ram.cpp
    src/ram_io.cpp
    src/mmu.cpp
    src/cp0.cpp
    src/cp1.cpp
    src/cpu.cpp
    src/machine_inspector.cpp
    src/machine.cpp
)

###########
## Tests ##
###########

add_executable(Tests
    test/test_main.cpp
    
    src/machine_inspector.cpp
# RAM
    test/test_ram.cpp
    src/ram.cpp
# Coprocessor 1
    test/test_cp1.cpp
    src/cp1.cpp
# CPU
    test/test_cpu.cpp
    src/cpu.cpp
    src/ram_io.cpp
    src/cp0.cpp
    src/mmu.cpp
	test/helpers/Terminal.cpp
	test/helpers/FileManager.cpp
# Machine Inspector
	test/test_save_restore_state.cpp
# RAMIO
    test/test_ram_io.cpp
# Example Programs - Kernel
	test/test_example_programs_kernel.cpp
)

############
## GLOBAL ##
############

target_compile_features(Tests PRIVATE cxx_std_17)
target_include_directories(Tests PRIVATE include third-party test/helpers)

target_compile_features(fs-mips32 PRIVATE cxx_std_17)
target_include_directories(fs-mips32 PRIVATE include)

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")

	target_compile_options(Tests PRIVATE -g -O0 --coverage -w)
    target_link_libraries(Tests PRIVATE -lgcov --coverage)

else() # Normal build

    if(MSVC)

        target_compile_options(Tests PRIVATE /W3 /fp:strict /wd4146 /wd4267 /permissive-)
        target_compile_definitions(Tests PRIVATE "-D_CRT_SECURE_NO_WARNINGS")

        target_compile_options(fs-mips32 PRIVATE /W3 /fp:strict /wd4146 /wd4267 /permissive-)
        target_compile_definitions(fs-mips32 PRIVATE "-D_CRT_SECURE_NO_WARNINGS")

    else() # GCC - Clang

        if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")

            target_compile_options(Tests PRIVATE
				-Wall -Wextra
				-Wno-logical-op-parentheses
				-Wno-bitwise-op-parentheses
				-Wno-implicit-fallthrough
				-Wno-dollar-in-identifier-extension
			)

            target_compile_options(fs-mips32 PRIVATE
				-Wall -Wextra
				-Wno-logical-op-parentheses
				-Wno-bitwise-op-parentheses
				-Wno-implicit-fallthrough
				-Wno-dollar-in-identifier-extension
			)
            
        else() # GCC

            target_compile_options(Tests PRIVATE
				-Wall -Wextra
				-Wno-parentheses
				-Wimplicit-fallthrough=0
				-Wno-unused-but-set-variable
			)

            target_compile_options(fs-mips32 PRIVATE
				-Wall -Wextra
				-Wno-parentheses
				-Wimplicit-fallthrough=0
				-Wno-unused-but-set-variable
			)

        endif()

        # x86 build
        target_compile_options(Tests PRIVATE "$<$<STREQUAL:${ARCH},x86>:-m32>")
        target_link_libraries(Tests PRIVATE "$<$<STREQUAL:${ARCH},x86>:-m32>")

        target_compile_options(fs-mips32 PRIVATE "$<$<STREQUAL:${ARCH},x86>:-m32>")
        target_link_libraries(fs-mips32 PRIVATE "$<$<STREQUAL:${ARCH},x86>:-m32>")

        # x86_64 build
        target_compile_options(Tests PRIVATE "$<$<STREQUAL:${ARCH},x64>:-m64>")
        target_link_libraries(Tests PRIVATE "$<$<STREQUAL:${ARCH},x64>:-m64>")
        
        target_compile_options(fs-mips32 PRIVATE "$<$<STREQUAL:${ARCH},x64>:-m64>")
        target_link_libraries(fs-mips32 PRIVATE "$<$<STREQUAL:${ARCH},x64>:-m64>")

		# Shared flags between GCC and Clang
        target_compile_options(Tests PRIVATE
			-mfpmath=sse -msse2
			-pedantic
			-Wno-unknown-pragmas
			-Wno-missing-braces
			-Wno-unused-variable
			-Wno-sign-compare
		)

        target_compile_options(fs-mips32 PRIVATE
			-mfpmath=sse -msse2
			-pedantic
			-Wno-unknown-pragmas
			-Wno-missing-braces
			-Wno-unused-variable
			-Wno-sign-compare
		)

    endif() # GCC - Clang
	
endif() # Debug|Release build

include(CTest)
enable_testing()
add_test(NAME AllTests COMMAND Tests)