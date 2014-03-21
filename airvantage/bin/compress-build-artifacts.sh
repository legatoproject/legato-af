#!/bin/sh

#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Romain Perier for Sierra Wireless - initial API and implementation
#*******************************************************************************


luavm=lua
lua_version=`${luavm} -e 'print(_VERSION)' | cut -d ' ' -f 2`
lua_srcdiet_version=

fail() {
    echo "FAIL!"
    exit 1
}

if [ $# != 1 ]; then
    echo "Usage: compress-build-artifacts [build_artifacts_dir]"
    exit 1
fi

if [ ! -d /opt/luasrcdiet ]; then
    echo "compress-build-artifacts: Warning: /opt/luasrcdiet not found, skipping..."
    exit 0
fi

# For some distributions like Arch Linux, /usr/bin/lua is the lastest Lua (5.2 for now).
# We want only Lua 5.1 here
if [ "$lua_version" != "5.1" ]; then
	which lua5.1 2>&1 >/dev/null

	if [ $? != 0 ]; then     
	    echo "Lua 5.1 is required !"
	    echo "Found version ${lua_version}"
	    exit 1
	fi
	luavm=lua5.1
	lua_version=`${luavm} -e 'print(_VERSION)' | cut -d ' ' -f 2`
fi

orig_dir=`pwd`
cd /opt/luasrcdiet
lua_srcdiet_version=`${luavm} LuaSrcDiet.lua -v | head -n 2 | tail -n 1 | cut -d ' ' -f 2`

echo "Found LuaVM ${lua_version}"
echo "Found LuaSrcDiet ${lua_srcdiet_version}"

for i in $(find $1 -name '*.lua'); do
    cp $i $1/tmp.lua
    echo -n "Minimizing $i..."
    ${luavm} LuaSrcDiet.lua --maximum $1/tmp.lua -o $i 2>&1 >/dev/null || fail
    echo "Ok"
done
rm -f $1/tmp.lua
cd $orig_dir
