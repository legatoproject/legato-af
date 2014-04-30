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

INCLUDE_DIRECTORIES(
    ${CRYPTO_SOURCE_DIR}
    ${LIB_HMAC_MD5_SOURCE_DIR}/hmac_rfc2104
    ${LIB_HMAC_MD5_SOURCE_DIR}/md5_rfc1321
    ${LIB_SHA1_SOURCE_DIR}
    ${LIB_MIHINI_KEYSTORE_SOURCE_DIR}
    ${LIB_OPENAES_SOURCE_DIR}
    ${LIB_TOMCRYPT_SOURCE_DIR}/headers
    ${MIHINI_TOMCRYPT_SOURCE_DIR})

ADD_LUA_LIBRARY(crypto_hmac ../hmac.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_hmac lib_hmac_md5 lib_sha1 lib_keystore lib_tomcrypt)
SET_TARGET_PROPERTIES(crypto_hmac PROPERTIES OUTPUT_NAME hmac)
INSTALL(TARGETS crypto_hmac LIBRARY DESTINATION lua/crypto)

ADD_LUA_LIBRARY(crypto_keystore ../keystore.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_keystore lib_keystore lib_tomcrypt)
SET_TARGET_PROPERTIES(crypto_keystore PROPERTIES OUTPUT_NAME keystore)
INSTALL(TARGETS crypto_keystore LIBRARY DESTINATION lua/crypto)

ADD_LUA_LIBRARY(crypto_random random.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_random lib_tomcrypt)
SET_TARGET_PROPERTIES(crypto_random PROPERTIES OUTPUT_NAME random)
INSTALL(TARGETS crypto_random LIBRARY DESTINATION lua/crypto)

ADD_LUA_LIBRARY(crypto_cipher lcipher.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_cipher lib_tomcrypt lib_keystore)
SET_TARGET_PROPERTIES(crypto_cipher PROPERTIES OUTPUT_NAME cipher)
INSTALL(TARGETS crypto_cipher LIBRARY DESTINATION lua/crypto)

ADD_LUA_LIBRARY(crypto_ecdh lecdh.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_ecdh lib_tomcrypt lib_tommath)
SET_TARGET_PROPERTIES(crypto_ecdh PROPERTIES OUTPUT_NAME ecdh)
INSTALL(TARGETS crypto_ecdh LIBRARY DESTINATION lua/crypto)

ADD_LUA_LIBRARY(crypto_aes aes.lua DESTINATION crypto)
ADD_DEPENDENCIES(crypto_aes crypto_cipher crypto_md5)
INSTALL(FILES aes.lua DESTINATION lua/crypto)

