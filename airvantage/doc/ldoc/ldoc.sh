#!/bin/sh

#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# and Eclipse Distribution License v1.0 which accompany this distribution.
#
# The Eclipse Public License is available at
#   http://www.eclipse.org/legal/epl-v10.html
# The Eclipse Distribution License is available at
#   http://www.eclipse.org/org/documents/edl-v10.php
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

BASEDIR=$(cd $(dirname $0) && pwd)
HOME_RA=$(cd $BASEDIR/../.. && pwd)
LUADOCUMENTOR_VERSION=0.1.2
LUADOCUMENTOR_REVISION=31a48059b3

# Get version from version.h, it is set by two define lines:
# #define MIHINI_AGENT__MAJOR_VERSION "42"
# #define MIHINI_AGENT__MINOR_VERSION "0"

extract_version_category ()
{
    # grep: get the correct line, sed: get and return only what is between the ""
    res=`grep "#define[[:space:]]\+MIHINI_AGENT__$1" $HOME_RA/agent/executable/version.h | sed s/".*\"\(.*\)\""/"\1"/ `
    # create a var MIHINI_$1 and affect it the value of res
    var="MIHINI_$1"
    eval ${var}="${res}"
}
extract_version_category MINOR
extract_version_category MAJOR
MIHINI_VERSION=$MIHINI_MAJOR.$MIHINI_MINOR

OUTPUT_DIR=$1/Lua
mkdir -p $1
rm -fr $OUTPUT_DIR

fail()
{
    rm -rf doctmp luadocumentor Lua_User_API_doc
    exit 1
}

link()
{
    if [ ! -f $1 ]; then
	echo "$1: No such file or directory"
	fail
    fi
    ln -s $1 $2 || fail
}

cd $BASEDIR

mkdir -p doctmp/utils/ltn12
mkdir doctmp/airvantage


# Lua Framework modules provided by SWI

link $HOME_RA/luafwk/sched/init.lua doctmp/sched.lua
link $HOME_RA/luafwk/log/init.lua doctmp/log.lua
link $HOME_RA/luafwk/utils/path.lua doctmp/utils/path.lua
link $HOME_RA/luafwk/utils/table.lua doctmp/utils/table.lua
link $HOME_RA/luafwk/utils/loader.lua doctmp/utils/loader.lua
link $HOME_RA/luafwk/utils/ltn12/source.lua doctmp/utils/ltn12/source.lua
link $HOME_RA/luafwk/checks/checks.c doctmp/checks.c
link $HOME_RA/luafwk/serial/serial.lua doctmp/serial.lua
link $HOME_RA/luafwk/persist/file.lua doctmp/persist.lua

link $HOME_RA/luafwk/serialframework/modbus/modbus.lua doctmp/modbus.lua
link $HOME_RA/luafwk/serialframework/modbus/modbustcp.lua doctmp/modbustcp.lua
link $HOME_RA/luafwk/timer.lua doctmp/timer.lua
link $HOME_RA/luafwk/niltoken.lua doctmp/niltoken.lua
link $HOME_RA/luafwk/returncodes.c doctmp/returncodes.c
link $HOME_RA/luafwk/gpio/gpio.c   doctmp/gpio.c
#link $HOME_RA/luafwk/gpio/gpio.lua doctmp/gpio.lua


link $HOME_RA/luafwk/racon/init.lua doctmp/airvantage.lua
link $HOME_RA/luafwk/racon/asset/init.lua doctmp/airvantage/asset.lua
link $HOME_RA/luafwk/racon/system.lua doctmp/system.lua
link $HOME_RA/luafwk/racon/sms.lua doctmp/sms.lua
link $HOME_RA/luafwk/racon/table.lua doctmp/airvantage/table.lua
link $HOME_RA/luafwk/racon/devicetree.lua doctmp/devicetree.lua

# Lua Framework modules from third parties

# Official Lua
cp lua-execution-environments/lua/5.1/api/*.lua doctmp/
link $HOME_RA/luafwk/lua/bit32.luadoc doctmp/bit32.lua

# LuaSocket
cp lua-execution-environments/luasocket/2.0/*.lua doctmp/

# Lfs
cp lua-execution-environments/lfs/1.5/*.lua doctmp/

# Various
link $HOME_RA/luafwk/lpack/lpack.c doctmp/lpack.c





ZIP=luadocumentor-${LUADOCUMENTOR_VERSION}-${LUADOCUMENTOR_REVISION}-noarch.zip

if [ ! -f $ZIP ]; then
    wget http://download.eclipse.org/koneki/luadocumentor/${LUADOCUMENTOR_VERSION}/${ZIP} || exit 1
fi
unzip $ZIP -d luadocumentor > /dev/null 2>&1 || exit 1



cd luadocumentor
lua luadocumentor.lua -f doc -s ../stylesheet.css -d ../Lua_User_API_doc ../doctmp || exit 1
lua luadocumentor.lua -f api -d ../Lua_User_API_doc ../doctmp || exit 1

cd ../Lua_User_API_doc
mkdir docs
mv *.html *.css docs

zip -9 api.zip *.lua || exit 1

cat > mihini.rockspec <<EOF
package = "mihini"
version = "${MIHINI_VERSION}"
flags = { ee = true }
description = {
   summary = "Mihini ${MIHINI_VERSION} Execution Environment",
   detailed = [[ Mihini ${MIHINI_VERSION} Execution Environment ]],
   licence = "EPL",
   homepage= "http://www.sierrawireless.com/en/productsandservices/AirPrime/Application_Framework/Libraries/AirVantage_Agent.aspx"
}
api = {
   file = "api.zip"
}
documentation ={
  dir="docs"
}
EOF

mkdir template
cat > template/main.lua <<EOF
local sched = require 'sched'
local shell = require 'shell.telnet'

-- Start a telnet server on port 1234
-- Once this program is started, you can start a Lua VM through telnet
-- using the following command: telnet localhost 1234
local function run_server()
  shell.init {
    address     = '0.0.0.0',
    port        = 1234,
    editmode    = "edit",
    historysize = 100 }
end

local function main()
  -- Create a thread to start the telnet server
  sched.run(run_server)
  -- Starting the sched main loop to execute the previous thread
  sched.loop()
end

main()

EOF



# Koneki limitations, don't remove "-r" and don't add "docs/*.html" as arguments
# Otherwises "zip" will generate an archive with wrong metadata
zip -r -9 mihini-${MIHINI_VERSION}.zip docs api.zip template *.rockspec || exit 1
rm api.zip *.rockspec
mv mihini-${MIHINI_VERSION}.zip  ../
cd ..
rm -rf doctmp luadocumentor template

cp -r Lua_User_API_doc/docs $OUTPUT_DIR
