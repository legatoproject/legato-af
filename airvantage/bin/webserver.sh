#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Fabien Fleutot for Sierra Wireless - initial API and implementation
#*******************************************************************************

#!/bin/bash


# usage: webserver.sh
# (no args needed)
# purpose: build and run Lua Platform Server
# output: If needed it will create a build.default folder in current working directory where it'll compile everything needed.
# You can also use it “over” an existing build.default folder (resulting of the use of build.sh): to do so you must call the webserver script with the build.default folder being in the current working dir.



#ROOT_DIR: location of Agent sources location
ROOT_DIR=$(cd $(dirname $0)/.. && pwd)
#EXEC_DIR: working directory where this script is executed
EXEC_DIR=$(pwd)


cd $EXEC_DIR
#check if build.default exists
if [ ! -d $EXEC_DIR/build.default ]; then
    #if build.default doesn't exist, call start.sh to create it and build default targets
    $ROOT_DIR/bin/build.sh
fi

cd $EXEC_DIR/build.default
# build all Lua Platform Server dependencies
make all web platformserver lua
cd runtime/lua
# start the Lua Platform Server
../bin/lua -l platform
