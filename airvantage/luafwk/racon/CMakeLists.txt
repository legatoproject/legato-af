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

PROJECT(MIHINI_RACON)

ADD_LUA_LIBRARY(racon DESTINATION racon
    =asset/init.lua =asset/tree.lua common.lua
    empparser.lua init.lua ipc.lua table.lua devicetree.lua)
ADD_LUA_LIBRARY(racon_system DESTINATION racon
    system.lua)
ADD_LUA_LIBRARY(racon_sms DESTINATION racon
    sms.lua)
ADD_DEPENDENCIES(racon yajl)
INSTALL(FILES common.lua empparser.lua init.lua ipc.lua table.lua system.lua devicetree.lua sms.lua DESTINATION lua/racon)
INSTALL(FILES asset/init.lua asset/tree.lua DESTINATION lua/racon/asset)
