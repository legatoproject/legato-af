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

PROJECT(CRYPTO)



INCLUDE_DIRECTORIES(
    ${CRYPTO_SOURCE_DIR}
    ${LIB_HMAC_MD5_SOURCE_DIR}/hmac_rfc2104
    ${LIB_HMAC_MD5_SOURCE_DIR}/md5_rfc1321
    ${LIB_SHA1_SOURCE_DIR}
    ${LIB_MIHINI_KEYSTORE_SOURCE_DIR}
    ${LIB_OPENAES_SOURCE_DIR})

ADD_LUA_LIBRARY(crypto_md5 md5.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_md5 lib_hmac_md5)
SET_TARGET_PROPERTIES(crypto_md5 PROPERTIES OUTPUT_NAME md5)
INSTALL(TARGETS crypto_md5 LIBRARY DESTINATION lua/crypto)

ADD_LUA_LIBRARY(crypto_sha1 sha1.c DESTINATION crypto)
TARGET_LINK_LIBRARIES(crypto_sha1 lib_sha1)
SET_TARGET_PROPERTIES(crypto_sha1 PROPERTIES OUTPUT_NAME sha1)
INSTALL(TARGETS crypto_sha1 LIBRARY DESTINATION lua/crypto)

ADD_SUBDIRECTORY (${SECURITY_LIB})
 
