#!/bin/bash

set -e

cd ../../../..
source ./bin/configlegatoenv
make localhost
cd -

#cd ../../../build/localhost
#make
#cd -


PATH=$PATH:../../../../build/localhost/bin


killall serviceDirectory || true

serviceDirectory &
sleep 1
logCtrlDaemon &


sudo mkdir -p /opt/legato/configTree/
sudo ../../../../build/localhost/bin/configTree

