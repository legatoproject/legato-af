#!/bin/bash

set -e

cd ../../..
source ./bin/configlegatoenv
make localhost
cd -

#cd ../../../build/localhost
#make
#cd -


PATH=$PATH:../../../build/localhost/bin


killall serviceDirectory || true

serviceDirectory &
logCtrlDaemon &


sudo mkdir -p /opt/legato/configTree/
sudo nemiver ../../../build/localhost/bin/configTree

