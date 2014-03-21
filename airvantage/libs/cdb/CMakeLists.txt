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

# Need to generate libs: {cdb,alloc,buffer,unix,byte}.a
# cdb.a    depends on  {cdb,cdb_hash,cdb_make}.c
# alloc.a  depends on alloc.c
# buffer.a depends on {strerr_sys,buffer,buffer_put,buffer_2,strerr_die,buffer_get,buffer_copy,buffer_1,buffer_0,buffer_1s}.c
# unix.a   depends on {error,open_read,open_trunc,error_str,seek_set,seek_cur}.c
# byte.a   depends on {byte_copy,byte_cr,str_len,fmt_ulong,uint32_unpack,byte_diff,uint32_pack,scan_ulong}.c

PROJECT(LIB_CDB)

INCLUDE_DIRECTORIES(${LIB_CDB_SOURCE_DIR})

SET(CDB_SRC cdb.c cdb_hash.c cdb_make.c)
SET(ALLOC_SRC alloc.c)
SET(BUFFER_SRC strerr_sys.c buffer.c buffer_put.c buffer_2.c strerr_die.c buffer_get.c buffer_copy.c buffer_1.c buffer_0.c buffer_1s.c)
SET(UNIX_SRC error.c open_read.c open_trunc.c error_str.c seek_set.c seek_cur.c)
SET(BYTE_SRC byte_copy.c byte_cr.c str_len.c fmt_ulong.c uint32_unpack.c byte_diff.c uint32_pack.c scan_ulong.c)

ADD_LIBRARY(lib_cdb_cdb    STATIC ${CDB_SRC})
ADD_LIBRARY(lib_cdb_alloc  STATIC ${ALLOC_SRC})
ADD_LIBRARY(lib_cdb_buffer STATIC ${BUFFER_SRC})
ADD_LIBRARY(lib_cdb_unix   STATIC ${UNIX_SRC})
ADD_LIBRARY(lib_cdb_byte   STATIC ${BYTE_SRC})

# necessary so that the lib can be used linked in a static code or a shared library
SET_TARGET_PROPERTIES(lib_cdb_cdb    PROPERTIES COMPILE_FLAGS -fPIC OUTPUT_NAME cdb_cdb)
SET_TARGET_PROPERTIES(lib_cdb_alloc  PROPERTIES COMPILE_FLAGS -fPIC OUTPUT_NAME cdb_alloc)
SET_TARGET_PROPERTIES(lib_cdb_buffer PROPERTIES COMPILE_FLAGS -fPIC OUTPUT_NAME cdb_buffer)
SET_TARGET_PROPERTIES(lib_cdb_unix   PROPERTIES COMPILE_FLAGS -fPIC OUTPUT_NAME cdb_unix)
SET_TARGET_PROPERTIES(lib_cdb_byte   PROPERTIES COMPILE_FLAGS -fPIC OUTPUT_NAME cdb_byte)
