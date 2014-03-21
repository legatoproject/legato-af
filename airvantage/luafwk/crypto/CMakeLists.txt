#*******************************************************************************
# Copyright (c) 2013 Sierra Wireless and others.
# All rights reserved. This building script is made available under the terms
# of the Eclipse Public License v1.0 which accompanies this distribution, and
# is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

PROJECT( CRYPTO)

INCLUDE_DIRECTORIES(
    ${LIB_HMAC_MD5_SOURCE_DIR}/hmac_rfc2104
    ${LIB_HMAC_MD5_SOURCE_DIR}/md5_rfc1321
    ${LIB_SHA1_SOURCE_DIR}
    ${LIB_MIHINI_KEYSTORE_SOURCE_DIR}
    ${LIB_OPENAES_SOURCE_DIR})

ADD_LUA_LIBRARY(crypto_md5 md5.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_md5 lib_hmac_md5)
SET_TARGET_PROPERTIES(crypto_md5 PROPERTIES OUTPUT_NAME md5)

ADD_LUA_LIBRARY(crypto_sha1 sha1.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_sha1 lib_sha1)
SET_TARGET_PROPERTIES(crypto_sha1 PROPERTIES OUTPUT_NAME sha1)

ADD_LUA_LIBRARY(crypto_hmac hmac.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_hmac lib_hmac_md5 lib_sha1 lib_keystore)
SET_TARGET_PROPERTIES(crypto_hmac PROPERTIES OUTPUT_NAME hmac)

ADD_LUA_LIBRARY(crypto_keystore keystore.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_keystore lib_keystore)
SET_TARGET_PROPERTIES(crypto_keystore PROPERTIES OUTPUT_NAME keystore)

ADD_LUA_LIBRARY(crypto_random random.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_random lib_openaes)
SET_TARGET_PROPERTIES(crypto_random PROPERTIES OUTPUT_NAME random)

ADD_LUA_LIBRARY(crypto_openaes_core openaes_core.c DESTINATION crypto/openaes)
TARGET_LINK_LIBRARIES(crypto_openaes_core lib_openaes)
SET_TARGET_PROPERTIES(crypto_openaes_core PROPERTIES OUTPUT_NAME core)

ADD_LUA_LIBRARY(crypto_aes aes.lua DESTINATION crypto)
INSTALL(TARGETS crypto_hmac LIBRARY DESTINATION lua/crypto)
INSTALL(FILES aes.lua DESTINATION lua/crypto)
INSTALL(TARGETS crypto_md5 crypto_sha1 crypto_keystore crypto_random LIBRARY DESTINATION lua/crypto)
INSTALL(TARGETS crypto_openaes_core LIBRARY DESTINATION lua/crypto/openaes)
