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
PROJECT(MIHINI_M3DA)



### Serialization & deserialization ###

# Low-level interface to C library
INCLUDE_DIRECTORIES(${LIB_MIHINI_BYSANT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${LIB_MIHINI_COMMON_SOURCE_DIR})
INCLUDE_DIRECTORIES(${LUA_SOURCE_DIR}) # TODO maybe not necessary

# Serialization is compiled as a DLL, separately from deserialize and the
# central luaopen_* entry, because it's also used by stagedb

ADD_LIBRARY(lib_bysant_lua SHARED
    bysant/bysant_core_serialize.c bysant/bysant_core_deserialize.c bysant/bysant_core_common.c)
TARGET_LINK_LIBRARIES(lib_bysant_lua lib_bysant)
SET_TARGET_PROPERTIES(lib_bysant_lua PROPERTIES OUTPUT_NAME bysant_lua)

# m3da.bysant.core only contains luaopen_m3da_bysant_core and a link to lib_bysant_lua

ADD_LUA_LIBRARY(bysant_core DESTINATION m3da/bysant bysant/bysant_core.c)
TARGET_LINK_LIBRARIES(bysant_core lib_bysant_lua)
SET_TARGET_PROPERTIES(bysant_core PROPERTIES OUTPUT_NAME core)

# High-level Lua serializer/deserializer
ADD_LUA_LIBRARY(m3da_bysant DESTINATION m3da bysant.lua)
ADD_DEPENDENCIES(m3da_bysant bysant_core)

### Communication layers ###

# Client-side transport layers
ADD_LUA_LIBRARY(m3da_transport DESTINATION m3da/transport transport/http.lua transport/tcp.lua)

# Server-side transport layers
ADD_LUA_LIBRARY(m3da_transport_servers DESTINATION
    m3da/transport transport/httpserver.lua transport/tcpserver.lua)

# Basic session communication layer
ADD_LUA_LIBRARY(m3da_session DESTINATION m3da/session session/default.lua)

# Authentication & encryption enabled session communication layer
IF(${SECURITY_LIB} STREQUAL "tomcrypt")
    ADD_LUA_LIBRARY(m3da_session_security DESTINATION m3da =session/security.lua =session/provisioning.lua)
    ADD_DEPENDENCIES(m3da_session_security crypto_aes crypto_random crypto_hmac crypto_ecdh crypto_md5)
ELSE(${SECURITY_LIB} STREQUAL "tomcrypt")
    ADD_LUA_LIBRARY(m3da_session_security DESTINATION m3da =session/security.lua)
    ADD_DEPENDENCIES(m3da_session_security crypto_aes crypto_random crypto_hmac)
ENDIF(${SECURITY_LIB} STREQUAL "tomcrypt")

INSTALL(TARGETS lib_bysant_lua LIBRARY DESTINATION lib)
INSTALL(TARGETS bysant_core LIBRARY DESTINATION lua/m3da/bysant)
INSTALL(FILES bysant.lua DESTINATION lua/m3da)
INSTALL(FILES session/default.lua session/security.lua DESTINATION lua/m3da/session)
INSTALL(FILES transport/httpserver.lua transport/tcpserver.lua transport/http.lua transport/tcp.lua DESTINATION lua/m3da/transport)
