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

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(LIB_MIHINI_LUASIGNAL)

INCLUDE_DIRECTORIES(${LIB_MIHINI_COMMON_SOURCE_DIR})

ADD_LIBRARY(lib_luasignal STATIC luasignal.c)
SET_TARGET_PROPERTIES(lib_luasignal PROPERTIES
    OUTPUT_NAME luasignal
    COMPILE_FLAGS -fPIC) # necessary so that the lib can be used linked in a static code or a shared library

