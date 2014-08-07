#*******************************************************************************
# Copyright (c) 2013 Sierra Wireless and others.
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

INCLUDE_DIRECTORIES( 
    ${CRYPTO_SOURCE_DIR}
    ${LIB_HMAC_MD5_SOURCE_DIR}/hmac_rfc2104
    ${LIB_HMAC_MD5_SOURCE_DIR}/md5_rfc1321
    ${LIB_SHA1_SOURCE_DIR}
    ${LIB_MIHINI_KEYSTORE_SOURCE_DIR}
    ${LIB_OPENAES_SOURCE_DIR})

ADD_LUA_LIBRARY(crypto_hmac ../hmac.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_hmac lib_hmac_md5 lib_sha1 lib_keystore)
SET_TARGET_PROPERTIES(crypto_hmac PROPERTIES OUTPUT_NAME hmac)
INSTALL(TARGETS crypto_hmac LIBRARY DESTINATION lua/crypto)

ADD_LUA_LIBRARY(crypto_keystore ../keystore.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_keystore lib_keystore)
SET_TARGET_PROPERTIES(crypto_keystore PROPERTIES OUTPUT_NAME keystore)
INSTALL(TARGETS crypto_keystore LIBRARY DESTINATION lua/crypto)

ADD_LUA_LIBRARY(crypto_random random.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_random lib_openaes)
SET_TARGET_PROPERTIES(crypto_random PROPERTIES OUTPUT_NAME random)
INSTALL(TARGETS crypto_random LIBRARY DESTINATION lua/crypto)

ADD_LUA_LIBRARY(crypto_openaes_core openaes_core.c DESTINATION crypto/openaes)
TARGET_LINK_LIBRARIES(crypto_openaes_core lib_openaes)
SET_TARGET_PROPERTIES(crypto_openaes_core PROPERTIES OUTPUT_NAME core)
INSTALL(TARGETS crypto_openaes_core LIBRARY DESTINATION lua/crypto/openaes)

ADD_LUA_LIBRARY(crypto_aes aes.lua DESTINATION crypto)
ADD_DEPENDENCIES(crypto_aes crypto_openaes_core crypto_md5)
INSTALL(FILES aes.lua DESTINATION lua/crypto)

