#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(JANSSON_INC_DIR "${CMAKE_BINARY_DIR}/3rdParty/jansson-2.6/include/")

mkexe(  update
            ../updateTool
            -i ../../c/src
            -i ${JANSSON_INC_DIR}
            -i ${PROJECT_SOURCE_DIR}/interfaces/modemServices
            --ldflags=-ljansson
        DEPENDS
            updateCtrl.c
            manifest.c
            ${PROJECT_SOURCE_DIR}/interfaces/modemServices/le_fwupdate.api
            Component.cdef
        )
