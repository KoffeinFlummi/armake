#!/bin/bash
# Config rapification/derapification

mkdir -p /tmp/amktest || exit 1

./bin/armake binarize -f test/rapification/config.cpp /tmp/amktest/config.bin
./bin/armake derapify -f /tmp/amktest/config.bin /tmp/amktest/config.cpp

cmp --silent test/rapification/config.cpp /tmp/amktest/config.cpp || {
    rm -rf /tmp/amktest
    exit 1
}

rm -rf /tmp/amktest
