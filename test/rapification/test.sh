#!/bin/bash
# Config rapification/derapification

mkdir -p /tmp/amktest || exit 1

./bin/armake binarize -f -w unquoted-string test/rapification/config.cpp /tmp/amktest/config.bin
./bin/armake derapify -f /tmp/amktest/config.bin /tmp/amktest/config.cpp

git diff --no-index -- test/rapification/config_ref.cpp /tmp/amktest/config.cpp > /dev/null || {
    rm -rf /tmp/amktest
    exit 1
}

rm -rf /tmp/amktest
