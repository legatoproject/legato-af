#!/bin/sh

#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

BASEDIR=$(cd $(dirname $0) && pwd)
HOME_RA=$(cd $BASEDIR/../.. && pwd)
LUADOCUMENTOR_VERSION=0.1.2
LUADOCUMENTOR_REVISION=31a48059b3
MIHINI_VERSION=0.9

while getopts g: o
do  case "$o" in
    g)  ga_tracker_path="$OPTARG";;
    ?)  print "Usage: $0 [-g ga_tracker_path]"
        exit 1;;
    esac
done


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

link $HOME_RA/luafwk/sched/init.lua doctmp/sched.lua
link $HOME_RA/luafwk/log/init.lua doctmp/log.lua
link $HOME_RA/luafwk/utils/path.lua doctmp/utils/path.lua
link $HOME_RA/luafwk/utils/table.lua doctmp/utils/table.lua
link $HOME_RA/luafwk/utils/loader.lua doctmp/utils/loader.lua
link $HOME_RA/luafwk/utils/ltn12/source.lua doctmp/utils/ltn12/source.lua
link $HOME_RA/luafwk/checks/checks.c doctmp/checks.c
link $HOME_RA/luafwk/serial/serial.lua doctmp/serial.lua
link $HOME_RA/luafwk/persist/file.lua doctmp/persist.lua
link $HOME_RA/luafwk/lpack/lpack.c doctmp/lpack.c
link $HOME_RA/luafwk/serialframework/modbus/modbus.lua doctmp/modbus.lua
link $HOME_RA/luafwk/serialframework/modbus/modbustcp.lua doctmp/modbustcp.lua
link $HOME_RA/luafwk/timer.lua doctmp/timer.lua
link $HOME_RA/luafwk/niltoken.lua doctmp/niltoken.lua

link $HOME_RA/luafwk/gpio/gpio.c   doctmp/gpio.c
#link $HOME_RA/luafwk/gpio/gpio.lua doctmp/gpio.lua


link $HOME_RA/luafwk/racon/init.lua doctmp/airvantage.lua
link $HOME_RA/luafwk/racon/asset/init.lua doctmp/airvantage/asset.lua
link $HOME_RA/luafwk/racon/system.lua doctmp/system.lua
link $HOME_RA/luafwk/racon/sms.lua doctmp/sms.lua
link $HOME_RA/luafwk/racon/table.lua doctmp/airvantage/table.lua
link $HOME_RA/luafwk/racon/devicetree.lua doctmp/devicetree.lua



link $HOME_RA/luafwk/lua/lua.luadoc doctmp/lua.lua
# link $HOME_RA/luafwk/linux/liblua/luadoc/coroutine.lua doctmp/liblua/coroutine.lua
# link $HOME_RA/luafwk/linux/liblua/luadoc/debug.lua doctmp/liblua/debug.lua
# link $HOME_RA/luafwk/linux/liblua/luadoc/global.lua doctmp/liblua/global.lua
# link $HOME_RA/luafwk/linux/liblua/luadoc/io.lua doctmp/liblua/io.lua
# link $HOME_RA/luafwk/linux/liblua/luadoc/math.lua doctmp/liblua/math.lua
# link $HOME_RA/luafwk/linux/liblua/luadoc/os.lua doctmp/liblua/os.lua
# link $HOME_RA/luafwk/linux/liblua/luadoc/package.lua doctmp/liblua/pakage.lua
# link $HOME_RA/luafwk/linux/liblua/luadoc/string.lua doctmp/liblua/string.lua
# link $HOME_RA/luafwk/linux/liblua/luadoc/table.lua doctmp/liblua/table.lua


#mkdir doctmp/socket
#mkdir doctmp/socket/socket

link $HOME_RA/luafwk/luasocket/luasocket.luadoc doctmp/socket.lua
# link $HOME_RA/luafwk/common/luasocket/linux/luadoc/socket.lua doctmp/socket/socket.lua
# link $HOME_RA/luafwk/common/luasocket/linux/luadoc/mime.lua doctmp/socket/mime.lua
# link $HOME_RA/luafwk/common/luasocket/linux/luadoc/ltn12.lua doctmp/socket/ltn12.lua
# link $HOME_RA/luafwk/common/luasocket/linux/luadoc/socket/url.lua doctmp/socket/socket/url.lua
# link $HOME_RA/luafwk/common/luasocket/linux/luadoc/socket/http.lua doctmp/socket/socket/http.lua
# link $HOME_RA/luafwk/common/luasocket/linux/luadoc/socket/smtp.lua doctmp/socket/socket/smtp.lua
# link $HOME_RA/luafwk/common/luasocket/linux/luadoc/socket/ftp.lua doctmp/socket/socket/ftp.lua


#mkdir doctmp/lfs

link $HOME_RA/luafwk/lfs/lfs.luadoc doctmp/lfs.lua

# mkdir doc/luafwk
# mkdir doc/airvantage
# mkdir doc/liblua
# mkdir doc/socket
# mkdir doc/lfs


ZIP=luadocumentor-${LUADOCUMENTOR_VERSION}-${LUADOCUMENTOR_REVISION}-noarch.zip

if [ ! -f $ZIP ]; then
    wget http://download.eclipse.org/koneki/luadocumentor/${LUADOCUMENTOR_VERSION}/${ZIP} || exit 1
fi
unzip $ZIP -d luadocumentor > /dev/null 2>&1 || exit 1



if [ -n "$ga_tracker_path" ]; then
      # Inserts Google Analytics header into luadocumentor page template.
      # at some point this custom template should be an option to luadocumentor.
      # For now, just insert GA header just before </head>.
      # The line breaks in sed command are made on purpose!
      sed "\,</head>, {
              h
              r $ga_tracker_path
              g
              N
      }" luadocumentor/template/page.lua > page.lua
      # It's likely that the sed command could have been done in one line.
      mv page.lua luadocumentor/template/page.lua
fi


cp lua5.1-execution-environment/lua/5.1/api/*.lua doctmp/

cd luadocumentor
lua luadocumentor.lua -f doc -s ../stylesheet.css -d ../Lua_User_API_doc ../doctmp || exit 1
lua luadocumentor.lua -f api -d ../Lua_User_API_doc ../doctmp || exit 1
# lua luadocumentor.lua -d ../doc/airvantage ../doctmp/airvantage
# lua luadocumentor.lua -d ../doc/liblua ../doctmp/liblua
# lua luadocumentor.lua -d ../doc/socket ../doctmp/socket
# lua luadocumentor.lua -d ../doc/lfs ../doctmp/lfs

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
