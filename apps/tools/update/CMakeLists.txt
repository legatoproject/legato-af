#***************************************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#***************************************************************************************************

find_package(Legato REQUIRED)

mkexe(  update
            ../update
            -i ${LEGATO_ROOT}/framework/c/src
            -i ${PROJECT_SOURCE_DIR}/interfaces/updateDaemon
        DEPENDS
            update.c
            ${PROJECT_SOURCE_DIR}/interfaces/updateDaemon/le_update.api
            Component.cdef
        )
