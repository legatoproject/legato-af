#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This building script is made available under the terms
# of the Eclipse Public License v1.0 which accompanies this distribution, and
# is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

PROJECT(CDB)

INCLUDE_DIRECTORIES(${LIB_CDB_SOURCE_DIR})

SET(CDB_LIBS lib_cdb_cdb lib_cdb_alloc lib_cdb_buffer lib_cdb_unix lib_cdb_byte)

ADD_LUA_LIBRARY(cdb lcdb.c)
TARGET_LINK_LIBRARIES(cdb ${CDB_LIBS})

ADD_LUA_LIBRARY(cdb_make lcdb_make.c)
TARGET_LINK_LIBRARIES(cdb_make ${CDB_LIBS})
INSTALL(TARGETS cdb cdb_make LIBRARY DESTINATION lua)
