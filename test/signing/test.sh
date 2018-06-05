#!/bin/bash
# PBO Signing

mkdir -p /tmp/amktest || exit 1

cp test/signing/*.pbo /tmp/amktest/

echo -e "\n\nTEST\tace_fcs\n----"
./bin/armake sign test/signing/*.biprivatekey /tmp/amktest/ace_fcs.pbo
echo -e "\n\nTEST\tace_vehiclelock\n----"
./bin/armake sign test/signing/*.biprivatekey /tmp/amktest/ace_vehiclelock.pbo
echo -e "\n\nTEST\tdbo_old_bike\n----"
./bin/armake sign test/signing/*.biprivatekey /tmp/amktest/dbo_old_bike.pbo
echo -e "\n\nTEST\tace_medical\n----"
./bin/armake sign test/signing/*.biprivatekey /tmp/amktest/ace_medical.pbo

cmp --silent test/signing/ace_fcs.pbo.*.bisign /tmp/amktest/ace_fcs.pbo.*.bisign || {
    cp -f /tmp/amktest/ace_medical.pbo.*.bisign test/signing/.errored/errored_ace_fcs.bisign
    rm -rf /tmp/amktest
    echo "ace_fcs"
    exit 1
}

cmp --silent test/signing/ace_vehiclelock.pbo.*.bisign /tmp/amktest/ace_vehiclelock.pbo.*.bisign || {
    cp -f /tmp/amktest/ace_medical.pbo.*.bisign test/signing/.errored/errored_ace_vehiclelock.bisign
    rm -rf /tmp/amktest
    echo "ace_vehiclelock"
    exit 1
}

cmp --silent test/signing/dbo_old_bike.pbo.*.bisign /tmp/amktest/dbo_old_bike.pbo.*.bisign || {
    cp -f /tmp/amktest/ace_medical.pbo.*.bisign test/signing/.errored/errored_dbo_old_bike.bisign
    rm -rf /tmp/amktest
    echo "dbo_old_bike"
    exit 1
}

cmp --silent test/signing/ace_medical.pbo.*.bisign /tmp/amktest/ace_medical.pbo.*.bisign || {
    cp -f /tmp/amktest/ace_medical.pbo.*.bisign test/signing/.errored/errored_ace_medical.bisign
    rm -rf /tmp/amktest
    echo "ace_medical"
    exit 1
}

rm -rf /tmp/amktest
