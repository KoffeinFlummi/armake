#!/bin/bash
# PBO Signing

mkdir -p /tmp/amktest || exit 1

cp test/signing/*.pbo /tmp/amktest/

./bin/armake sign test/signing/*.biprivatekey /tmp/amktest/ace_fcs.pbo
./bin/armake sign test/signing/*.biprivatekey /tmp/amktest/ace_vehiclelock.pbo
./bin/armake sign test/signing/*.biprivatekey /tmp/amktest/dbo_old_bike.pbo
./bin/armake sign test/signing/*.biprivatekey /tmp/amktest/ace_medical.pbo

cmp --silent test/signing/ace_fcs.pbo.*.bisign /tmp/amktest/ace_fcs.pbo.*.bisign || {
    rm -rf /tmp/amktest
    echo "ace_fcs"
    exit 1
}

cmp --silent test/signing/ace_vehiclelock.pbo.*.bisign /tmp/amktest/ace_vehiclelock.pbo.*.bisign || {
    rm -rf /tmp/amktest
    echo "ace_vehiclelock"
    exit 1
}

cmp --silent test/signing/dbo_old_bike.pbo.*.bisign /tmp/amktest/dbo_old_bike.pbo.*.bisign || {
    rm -rf /tmp/amktest
    echo "dbo_old_bike"
    exit 1
}

cmp --silent test/signing/ace_medical.pbo.*.bisign /tmp/amktest/ace_medical.pbo.*.bisign || {
    rm -rf /tmp/amktest
    echo "ace_medical"
    exit 1
}

rm -rf /tmp/amktest
