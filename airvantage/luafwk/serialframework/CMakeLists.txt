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

PROJECT(MIHINI_SERIAL_FRAMEWORK)


SET(MODBUS_SRC
    modbus/lua_modbus_serializer.c
    common/lua_serial_fwk.c
)

INCLUDE_DIRECTORIES(
    ${LIB_MIHINI_COMMON_SOURCE_DIR}
    ${LIB_MIHINI_SERIAL_FRAMEWORK_SOURCE_DIR}/common/
    ${LIB_MODBUS_SOURCE_DIR}
    ${MIHINI_SERIAL_FRAMEWORK_SOURCE_DIR}/common/
)

# TODO: move serial parts out of common in modbus.lua

# Transport-independent part
ADD_LUA_LIBRARY(modbus_serializer DESTINATION modbus EXCLUDE_FROM_ALL ${MODBUS_SRC})
SET_TARGET_PROPERTIES(modbus_serializer PROPERTIES OUTPUT_NAME serializer)
TARGET_LINK_LIBRARIES(modbus_serializer lib_modbus lib_returncodes)
ADD_DEPENDENCIES(modbus_serializer sched serial)

# Modbus over UART
ADD_LUA_LIBRARY(modbus_serial EXCLUDE_FROM_ALL modbus/modbus.lua)
ADD_DEPENDENCIES(modbus_serial modbus_serializer)

# Modbus over TCP
ADD_LUA_LIBRARY(modbus_tcp EXCLUDE_FROM_ALL modbus/modbustcp.lua)
ADD_DEPENDENCIES(modbus_tcp modbus_serializer socket_sched pack)

# Test
ADD_LUA_LIBRARY(test_modbusserializer DESTINATION tests EXCLUDE_FROM_ALL
                test/modbusserializer.lua)
ADD_DEPENDENCIES(test_modbusserializer modbus_serializer)

INSTALL(FILES modbus/modbus.lua modbus/modbustcp.lua DESTINATION lua)
INSTALL(TARGETS modbus_serializer LIBRARY DESTINATION lua/modbus)
