#!/bin/bash

failed=0

if [[ $# -ge 1 ]]; then
    target="${1/test\-//}"

    name=$(sed -n 2p ./test/$target/test.sh | tail -c +3)
    echo -n " TEST $name: "

    ./test/$target/test.sh

    if [ $? -eq 0 ]; then
        echo -e "\033[32mPASSED\033[0m"
    else
        echo -e "\033[31mFAILED\033[0m"
        failed=$((failed + 1))
    fi
    exit $failed
fi

for d in ./test/*/ ; do
    name=$(sed -n 2p $d/test.sh | tail -c +3)
    echo -n " TEST $name: "

    $d/test.sh

    if [ $? -eq 0 ]; then
        echo -e "\033[32mPASSED\033[0m"
    else
        echo -e "\033[31mFAILED\033[0m"
        failed=$((failed + 1))
    fi
done

exit $failed
