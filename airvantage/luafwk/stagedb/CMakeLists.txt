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

PROJECT(MIHINI_STAGEDB)

INCLUDE_DIRECTORIES(${LIB_MIHINI_BYSANT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${MIHINI_M3DA_SOURCE_DIR}/bysant)
INCLUDE_DIRECTORIES(${LIB_MIHINI_COMMON_SOURCE_DIR})
INCLUDE_DIRECTORIES(${LUA_SOURCE_DIR})

ADD_LUA_LIBRARY(stagedb stagedb_core.c stagedb.lua)
TARGET_LINK_LIBRARIES(stagedb lib_stagedb lib_bysant_lua)
ADD_DEPENDENCIES(stagedb checks ltn12 m3da_bysant)

INSTALL(TARGETS stagedb LIBRARY DESTINATION lua)
INSTALL(FILES stagedb.lua DESTINATION lua)
