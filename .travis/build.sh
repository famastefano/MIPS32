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
    if [[ -v SANITIZER ]]; then
		mkdir build-address
		mkdir build-thread
		mkdir build-memory
		mkdir build-undefined

		export SANITIZER
		
		cmake . -Bbuild-address -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH -DSANITIZE=address
		cmake . -Bbuild-thread -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH -DSANITIZE=thread
		cmake . -Bbuild-memory -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH -DSANITIZE=memory
		cmake . -Bbuild-undefined -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH -DSANITIZE=undefined
		
		cd build-address && make
		cd ../build-thread && make
		cd ../build-memory && make
		cd ../build-undefined && make

		cd ../build-address
		ctest -C Debug --output-on-failure
		cd ../build-thread
		ctest -C Debug --output-on-failure
		cd ../build-memory
		ctest -C Debug --output-on-failure
		cd ../build-undefined
		ctest -C Debug --output-on-failure
	else
		mkdir build-debug
		mkdir build-release
		
		cmake . -Bbuild-debug -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH
		cmake . -Bbuild-release -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DARCH=$ARCH

		make -C build-debug
		make -C build-release

		cd build-debug
		ctest -C Debug --output-on-failure

		cd ../build-release
		ctest -C Release --output-on-failure
	fi
fi