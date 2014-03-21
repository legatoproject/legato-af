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
PROJECT(LIB_MIHINI_KEYSTORE)

INCLUDE_DIRECTORIES(
    ${LIB_MIHINI_COMMON_SOURCE_DIR}
    ${LIB_HMAC_MD5_SOURCE_DIR}/hmac_rfc2104
    ${LIB_HMAC_MD5_SOURCE_DIR}/md5_rfc1321)

#SET(KEYSTORE_SRC keystore.c aes_obfuscator.c get_obfuscation_bin_key.c hmac_md5_rfc.c)
SET(KEYSTORE_SRC keystore.c dummy_obfuscator.c hmac_md5_rfc.c)

ADD_LIBRARY(lib_keystore SHARED ${KEYSTORE_SRC})
# TARGET_LINK_LIBRARIES(lib_keystore m)
SET_TARGET_PROPERTIES(lib_keystore PROPERTIES COMPILE_FLAGS -fPIC OUTPUT_NAME keystore)

TARGET_LINK_LIBRARIES(lib_keystore lib_hmac_md5)

ADD_CUSTOM_COMMAND(TARGET lib_keystore POST_BUILD
    COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/crypto
)

ADD_LIBRARY(lib_keystore_static EXCLUDE_FROM_ALL STATIC ${KEYSTORE_SRC})
TARGET_LINK_LIBRARIES(lib_keystore_static m)
SET_TARGET_PROPERTIES(lib_keystore_static PROPERTIES COMPILE_FLAGS -fPIC OUTPUT_NAME keystore_static)
INSTALL(TARGETS lib_keystore LIBRARY DESTINATION lib)
