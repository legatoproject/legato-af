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

# TODO move up one directory

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(LIB_MODBUS)

SET(LIB_MODBUS_SRC
    ${LIB_MODBUS_SOURCE_DIR}/modbus_serializer.c
)

INCLUDE_DIRECTORIES(
    ${LIB_MIHINI_COMMON_SOURCE_DIR}
    ${LIB_MIHINI_SERIAL_FRAMEWORK_SOURCE_DIR}/common
    ${LIB_MIHINI_SERIAL_FRAMEWORK_SOURCE_DIR}/common/oat
    ${LIB_MODBUS_SOURCE_DIR}
)

ADD_LIBRARY(lib_modbus STATIC EXCLUDE_FROM_ALL ${LIB_MODBUS_SRC})
SET_TARGET_PROPERTIES(lib_modbus PROPERTIES
    OUTPUT_NAME modbus # TODO not sure of the name
    COMPILE_FLAGS -fPIC) # necessary so that the lib can be used linked in a static code or a shared library
