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

PROJECT(MIHINI_SCHED)


INCLUDE_DIRECTORIES(${LIB_MIHINI_LUASIGNAL_SOURCE_DIR})
INCLUDE_DIRECTORIES(${LIB_MIHINI_COMMON_SOURCE_DIR})

ADD_LUA_LIBRARY(sched_exec DESTINATION sched exec.lua)

ADD_LUA_LIBRARY(sched_exec_core DESTINATION sched/exec exec_core.c)
TARGET_LINK_LIBRARIES(sched_exec_core lib_luasignal pthread)
SET_TARGET_PROPERTIES (sched_exec_core PROPERTIES OUTPUT_NAME core)
ADD_DEPENDENCIES(sched_exec sched_exec_core)

ADD_LUA_LIBRARY(sched_platform DESTINATION sched platform.lua fd.lua)
ADD_DEPENDENCIES(sched_platform coxpcall utils log checks sched_exec)

ADD_LUA_LIBRARY(sched_posixsignal DESTINATION sched lposixsignal.c)
ADD_DEPENDENCIES(sched_posixsignal sched)
SET_TARGET_PROPERTIES (sched_posixsignal PROPERTIES OUTPUT_NAME posixsignal)

ADD_LUA_LIBRARY(sched DESTINATION sched init.lua timer.lua pipe.lua lock.lua)
ADD_DEPENDENCIES(sched sched_platform)

ADD_LUA_LIBRARY(sched_daemon DESTINATION sched daemon.lua)
ADD_DEPENDENCIES(sched_daemon sched)


ADD_LUA_LIBRARY(sched_timer_core DESTINATION sched/timer timer_core.c)
TARGET_LINK_LIBRARIES(sched_timer_core lualib rt)
SET_TARGET_PROPERTIES (sched_timer_core PROPERTIES OUTPUT_NAME core)
ADD_DEPENDENCIES(sched sched_timer_core)

INSTALL(TARGETS sched_posixsignal LIBRARY DESTINATION lua/sched)
INSTALL(TARGETS sched_timer_core LIBRARY DESTINATION lua/sched/timer)
INSTALL(TARGETS sched_exec_core LIBRARY DESTINATION lua/sched/exec)
INSTALL(FILES exec.lua timer.lua pipe.lua daemon.lua platform.lua fd.lua lock.lua init.lua DESTINATION lua/sched)
