#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkexe(  execInApp
            ../execInApp
            --cflags=-DNO_LOG_CONTROL
            -i ${PROJECT_SOURCE_DIR}/framework/c/src
            -i ${PROJECT_SOURCE_DIR}/interfaces/supervisor
            -i ${PROJECT_SOURCE_DIR}/framework/c/src/security
            -i ${PROJECT_SOURCE_DIR}/framework/c/src/serviceDirectory
            -s ${PROJECT_SOURCE_DIR}/framework/c/src
            -v
            ${CFLAGS_TARGET_IMPORTS}
        DEPENDS
            execInApp.c
            Component.cdef
            ../../c/src/security/*
            )
