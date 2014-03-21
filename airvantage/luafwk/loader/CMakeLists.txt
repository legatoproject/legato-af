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

PROJECT(AAF_LOADER)

#define default value for loader module
IF(NOT LOADER_PATH_BASE)
    SET(LOADER_PATH_BASE "${CMAKE_INSTALL_PREFIX}/apps")
ENDIF(NOT LOADER_PATH_BASE)

ADD_DEFINITIONS(-DLOADER_PATH_BASE="${LOADER_PATH_BASE}")

ADD_LUA_LIBRARY(loader EXCLUDE_FROM_ALL loader.c)
INSTALL(TARGETS loader LIBRARY DESTINATION lua)
