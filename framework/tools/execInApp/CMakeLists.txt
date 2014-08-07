#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkexe(  execInApp
            ../execInApp
            --ldflags=-lsecurity
            -i ${PROJECT_SOURCE_DIR}/framework/c/src
            -i ${PROJECT_SOURCE_DIR}/interfaces/supervisor
            -i ${PROJECT_SOURCE_DIR}/framework/c/src/security
            -i ${PROJECT_SOURCE_DIR}/framework/c/src/serviceDirectory
            -s ${PROJECT_SOURCE_DIR}/framework/c/src
            -v
        DEPENDS
            execInApp.c
            Component.cdef
            security
            )
