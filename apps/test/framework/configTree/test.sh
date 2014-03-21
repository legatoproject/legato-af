#!/bin/bash

set -e

cd ../../../..
source ./bin/configlegatoenv
cd -


rm -rf ./build/*


mkexe configTest.c -o configTest -i ../../../../interfaces/config/c ../../../../build/localhost/bin/lib/lible_cfg_api.so -w ./build -v -l .

# ../../../build/localhost/bin/log level DEBUG "configTree/*"


./configTest
rm configTest libconfigTestDefault.so


sudo cat /opt/legato/configTree/*

