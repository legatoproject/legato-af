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
#!/bin/sh
AGENT_DIR=$(dirname $0)

export LUA_PATH="lua/?.lua;lua/?/init.lua"
export LUA_CPATH="lua/?.so"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib

cd $AGENT_DIR && bin/agent
