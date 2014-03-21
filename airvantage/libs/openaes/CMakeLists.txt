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

PROJECT(LIB_OPENAES)

INCLUDE_DIRECTORIES(${LIB_OPENAES_SOURCE_DIR})
INCLUDE_DIRECTORIES(${LIB_OPENAES_SOURCE_DIR}/isaac)
INCLUDE_DIRECTORIES(${LIB_MD5_SOURCE_DIR})

ADD_LIBRARY(lib_openaes SHARED oaes_lib.c isaac/rand.c)
SET_TARGET_PROPERTIES(lib_openaes PROPERTIES
    OUTPUT_NAME openaes
    COMPILE_FLAGS -fPIC) # necessary so that the lib can be used linked in a static code or a shared library
TARGET_LINK_LIBRARIES(lib_openaes lib_keystore)
INSTALL(TARGETS lib_openaes LIBRARY DESTINATION lib)
