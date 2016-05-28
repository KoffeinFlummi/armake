#!/bin/bash
# PBO Signing

mkdir -p /tmp/amktest || exit 1

cp test/signing/*.pbo /tmp/amktest/

./bin/armake sign test/signing/*.biprivatekey /tmp/amktest/ace_fcs.pbo
./bin/armake sign test/signing/*.biprivatekey /tmp/amktest/ace_vehiclelock.pbo

cmp --silent test/signing/ace_fcs.pbo.*.bisign /tmp/amktest/ace_fcs.pbo.*.bisign || {
    rm -rf /tmp/amktest
    exit 1
}

cmp --silent test/signing/ace_vehiclelock.pbo.*.bisign /tmp/amktest/ace_vehiclelock.pbo.*.bisign || {
    rm -rf /tmp/amktest
    exit 1
}

rm -rf /tmp/amktest
