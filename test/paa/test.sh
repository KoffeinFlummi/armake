#!/bin/bash
# PAA conversion

mkdir -p /tmp/amktest || exit 1

./bin/armake img2paa test/paa/test.png /tmp/amktest/test.paa
./bin/armake img2paa test/paa/test_alpha.png /tmp/amktest/test_alpha.paa

./bin/armake paa2img /tmp/amktest/test.paa /tmp/amktest/cmp.png
./bin/armake paa2img /tmp/amktest/test_alpha.paa /tmp/amktest/cmp_alpha.png

compare -metric AE -fuzz 5% test/paa/test.png /tmp/amktest/cmp.png /dev/null 2> /dev/null || {
    rm -rf /tmp/amktest
    exit 1
}

compare -metric AE -fuzz 8% test/paa/test_alpha.png /tmp/amktest/cmp_alpha.png /dev/null 2> /dev/null || {
    rm -rf /tmp/amktest
    exit 1
}

rm -rf /tmp/amktest
