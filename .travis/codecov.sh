if [[ -v COVERAGE ]]; then
    echo -e "\n\t\tUPLOADING TO CODECOV\n"
    bash <(curl -s https://codecov.io/bash) -X coveragepy -x "/usr/bin/gcov-7"
fi