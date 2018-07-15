if [ -z ${COVERAGE+x} ]; then
    cmake . -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON && make && ctest -C Debug --output-on-failure
else
    cmake . -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug && make && ctest -C Debug --output-on-failure &&
    cmake . -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release && make && ctest -C Release --output-on-failure;
fi;