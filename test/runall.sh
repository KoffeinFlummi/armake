#!/bin/bash

for d in ./test/*/ ; do
    name=$(sed -n 2p $d/test.sh | tail -c +3)
    echo -n " TEST $name: "

    $d/test.sh

    if [ $? -eq 0 ]; then
        echo -e "\033[32mPASSED\033[0m"
    else
        echo -e "\033[31mFAILED\033[0m"
    fi
done
