#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Laurent Barthelemy for Sierra Wireless - initial API and implementation
#     Fabien Fleutot     for Sierra Wireless - initial API and implementation
#*******************************************************************************
#!/bin/sh

ARG2=${2:-"$1/.."}
CMD1="local c = require'createpkg'; local res, dirpath, outputdir = c.createpkg('"$1"' ,'"$ARG2"'); "
CMD2="if not res then print('error:', dirpath) else print(string.format('Done, package files from [%s] created in [%s]', dirpath, outputdir)) end "
LUA_PATH="$(cd $(dirname "$0") && pwd)/?.lua" lua -e "$CMD1 $CMD2"

