#!/bin/bash

mkdir -p /tmp/amktest/sample || exit 1
mkdir -p /tmp/amktest/unpacked

head -c 256 < /dev/urandom > /tmp/amktest/sample/foo
./bin/armake build -f /tmp/amktest/sample /tmp/amktest/foo.pbo
./bin/armake unpack -f /tmp/amktest/foo.pbo /tmp/amktest/unpacked

cmp --silent /tmp/amktest/sample/foo /tmp/amktest/unpacked/foo || {
    rm -rf /tmp/amktest
    echo -e " TEST PBO packing/unpacking: \033[31mFAILED\033[0m"
    exit 1
}

rm -rf /tmp/amktest
echo -e " TEST PBO packing/unpacking: \033[32mPASSED\033[0m"
