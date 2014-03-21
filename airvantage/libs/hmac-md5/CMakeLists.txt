#*******************************************************************************
# Copyright (c) 2013 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************
PROJECT(LIB_HMAC_MD5)

INCLUDE_DIRECTORIES(
    ${LIB_HMAC_MD5_SOURCE_DIR}/hmac_rfc2104
    ${LIB_HMAC_MD5_SOURCE_DIR}/md5_rfc1321)

ADD_LIBRARY(lib_hmac_md5 SHARED md5_rfc1321/md5.c hmac_rfc2104/hmac-md5.c)
SET_TARGET_PROPERTIES(lib_hmac_md5 PROPERTIES COMPILE_FLAGS -fPIC OUTPUT_NAME hmacmd5)
INSTALL(TARGETS lib_hmac_md5 LIBRARY DESTINATION lib)
