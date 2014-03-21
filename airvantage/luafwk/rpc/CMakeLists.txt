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

PROJECT(MIHINI_RPC)

ADD_LUA_LIBRARY(rpc_common DESTINATION rpc common.lua init.lua proxy.lua builtinsignatures.lua)
ADD_DEPENDENCIES(rpc_common utils pack coxcall checks luatobin)

ADD_LUA_LIBRARY(rpc_sched DESTINATION rpc sched.lua)
ADD_DEPENDENCIES(rpc_sched rpc_common socket_sched)

ADD_LUA_LIBRARY(rpc_nosched DESTINATION rpc nosched.lua)
ADD_DEPENDENCIES(rpc_nosched rpc_common socket)

INSTALL(FILES builtinsignatures.lua common.lua init.lua nosched.lua proxy.lua sched.lua DESTINATION lua/rpc)
