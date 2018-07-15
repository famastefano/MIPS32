if [ -z ${COVERAGE+x} ]; then
# Creating report
    cd ${TRAVIS_BUILD_DIR}
    lcov --directory . --capture --output-file coverage.info coverage info
    lcov --remove coverage.info '/usr/*' --output-file coverage.info 
    lcov --list coverage.info
# Uploading report to CodeCov
    bash <(curl -s https://codecov.io/bash) || echo "Codecov did not collect coverage reports"
fi;