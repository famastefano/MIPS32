if [[ -v COVERAGE ]]; then
    cmake . -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Coverage && make
    echo -e "\n\t\tINITIALIZING LCOV ENVIRONMENT\n"
    lcov --version
    lcov -d . --zerocounters
    lcov -c -i -d . -o coverage_base.info --gcov-tool '/usr/bin/gcov-7' --no-external
    lcov --remove coverage_base.info '/usr/*' 'third-party/catch.hpp' 'test/test_ram.cpp' 'test/test_cp1.cpp' -o coverage_base.info
    lcov --list coverage_total.info
    echo -e "\n\t\tRUNNING TESTS\n"
    ctest -C Debug --output-on-failure
    echo -e "\n\t\tRUNNING LCOV AFTER TESTS\n"
    lcov -d . -c -o coverage_test.info --gcov-tool '/usr/bin/gcov-7' --no-external
    lcov -r coverage_base.info '/usr/*' 'third-party/catch.hpp' 'test/test_ram.cpp' 'test/test_cp1.cpp' -o coverage_test.info
    lcov -a coverage_base.info -a coverage_test.info -o coverage_total.info
    lcov --list coverage_total.info
else
    mkdir build-debug && mkdir build-release
	if [[ -v SANITIZER ]]; then
		cmake . -Bbuild-debug -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH -DSANITIZER=ON
	else
		cmake . -Bbuild-debug -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH
		cmake . -Bbuild-release -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DARCH=$ARCH
	fi
    
    cd build-debug && make && file Tests && ctest -C Debug --output-on-failure
    cd ..
    cd build-release && make && file Tests && ctest -C Release --output-on-failure
fi