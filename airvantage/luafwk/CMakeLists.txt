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

PROJECT(MIHINI_LUAFWK)

INCLUDE_DIRECTORIES(lua)

ADD_SUBDIRECTORY (tests)
ADD_SUBDIRECTORY (serialframework)

ADD_SUBDIRECTORY (checks)
ADD_SUBDIRECTORY (lpack)
ADD_SUBDIRECTORY (luatobin)
ADD_SUBDIRECTORY (messaging)
ADD_SUBDIRECTORY (shell)
ADD_SUBDIRECTORY (log)
ADD_SUBDIRECTORY (luasocket)
ADD_SUBDIRECTORY (ntp)
ADD_SUBDIRECTORY (sched)
ADD_SUBDIRECTORY (utils)
ADD_SUBDIRECTORY (web)
ADD_SUBDIRECTORY (rpc)

ADD_SUBDIRECTORY (coxpcall)
ADD_SUBDIRECTORY (lfs)
ADD_SUBDIRECTORY (loader)
ADD_SUBDIRECTORY (lua)
ADD_SUBDIRECTORY (serial)
ADD_SUBDIRECTORY (gpio)
ADD_SUBDIRECTORY (luacdb)
ADD_SUBDIRECTORY (persist)
ADD_SUBDIRECTORY (lua-yajl)
ADD_SUBDIRECTORY (crypto)

ADD_SUBDIRECTORY (racon)

# Some different names have been chosen for Racon services. They're aliased here:
# "Core" Racon modules aliases
ADD_LUA_LIBRARY(racon_aliases airvantage.lua devicetree.lua)
ADD_DEPENDENCIES(racon_aliases racon)
# System and Sms API aliases can be installed separately (like corresponding racon_system/sms targets)
ADD_LUA_LIBRARY(racon_alias_sms sms.lua )
ADD_LUA_LIBRARY(racon_alias_system system.lua )
ADD_DEPENDENCIES(racon_alias_sms racon_sms)
ADD_DEPENDENCIES(racon_alias_system racon_system)




ADD_LUA_LIBRARY(mihini_misc
    print.lua strict.lua timer.lua unittest.lua niltoken.lua)

# TODO move to platform
ADD_LUA_LIBRARY(mihini_linux fdwrapper.lua posixsignal.lua)

ADD_DEPENDENCIES(mihini_linux sched_posixsignal)

INCLUDE_DIRECTORIES(liblua)

ADD_SUBDIRECTORY(m3da)
ADD_SUBDIRECTORY(stagedb)

ADD_LUA_LIBRARY(debugger debugger.lua debugintrospection.lua)

INCLUDE_DIRECTORIES(${LIB_MIHINI_COMMON_SOURCE_DIR})
ADD_LUA_LIBRARY(returncodes returncodes.c)
TARGET_LINK_LIBRARIES(returncodes lib_returncodes)

ADD_CUSTOM_TARGET(luafwk)
ADD_DEPENDENCIES(luafwk mihini_misc mihini_linux bit32 luasignalcmd appmon_daemon serial racon_aliases racon_alias_sms racon_alias_system status)
INSTALL(FILES niltoken.lua posixsignal.lua strict.lua sms.lua serial/at.lua devicetree.lua fdwrapper.lua airvantage.lua system.lua unittest.lua debugger.lua debugintrospection.lua timer.lua print.lua DESTINATION lua)
INSTALL(TARGETS returncodes LIBRARY DESTINATION lua)

