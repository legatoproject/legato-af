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

PROJECT(YAJL)

INCLUDE_DIRECTORIES(${LIB_YAJL_SOURCE_DIR}/include)

ADD_LUA_LIBRARY(yajl lua_yajl.c)
SET_TARGET_PROPERTIES(yajl PROPERTIES OUTPUT_NAME yajl)
TARGET_LINK_LIBRARIES(yajl lib_yajl)
ADD_DEPENDENCIES(yajl niltoken)
INSTALL(TARGETS yajl LIBRARY DESTINATION lua)
