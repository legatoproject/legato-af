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

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(LIB_MIHINI_BYSANT)

INCLUDE_DIRECTORIES(${LIB_MIHINI_COMMON_SOURCE_DIR})

SET(BYSANT_SRC
    bysants.c
    bysantd.c
    bysant_common.c
    bysant_class.c
    ${LIB_MIHINI_COMMON_SOURCE_DIR}/rc_endian.c)

ADD_LIBRARY(lib_bysant SHARED ${BYSANT_SRC})
TARGET_LINK_LIBRARIES(lib_bysant m)
SET_TARGET_PROPERTIES(lib_bysant PROPERTIES
    COMPILE_FLAGS -fPIC
    OUTPUT_NAME bysant
    )

SET(SDB_SRC
  sdb_table.c
  sdb_read.c
  sdb_write.c
  sdb_serialize.c
  sdb_consolidate.c)

ADD_LIBRARY(lib_stagedb SHARED ${SDB_SRC})
TARGET_LINK_LIBRARIES(lib_stagedb lib_bysant bysant_core)
SET_TARGET_PROPERTIES(lib_stagedb PROPERTIES
    OUTPUT_NAME stagedb)
INSTALL(TARGETS lib_bysant LIBRARY DESTINATION lib)
INSTALL(TARGETS lib_stagedb LIBRARY DESTINATION lib)
