#!/bin/bash
# PBO packing/unpacking

mkdir -p /tmp/amktest/sample || exit 1
mkdir -p /tmp/amktest/unpacked

head -c 256 < /dev/urandom > /tmp/amktest/sample/foo
./bin/armake build -f /tmp/amktest/sample /tmp/amktest/foo.pbo
./bin/armake unpack -f /tmp/amktest/foo.pbo /tmp/amktest/unpacked

cmp --silent /tmp/amktest/sample/foo /tmp/amktest/unpacked/foo || {
    rm -rf /tmp/amktest
    exit 1
}

rm -rf /tmp/amktest
