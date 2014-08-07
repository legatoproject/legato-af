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

# CMakeLists for the lua linux version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(LIB_EXTVARS)


INCLUDE_DIRECTORIES(
    ${LIB_MIHINI_COMMON_SOURCE_DIR}
    ${LIB_EXTVARS_COMMON_SOURCE_DIR}
    ${LIB_MIHINI_LUASIGNAL_SOURCE_DIR}
)


ADD_LUA_LIBRARY(extvars DESTINATION agent/treemgr/handlers extvars.c)
TARGET_LINK_LIBRARIES(extvars dl pthread lib_luasignal lib_returncodes)

ADD_SUBDIRECTORY (treehdlsample)
INSTALL(TARGETS extvars LIBRARY DESTINATION lua/agent/treemgr/handlers)
