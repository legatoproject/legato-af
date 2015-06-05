#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkexe(  execInApp
            ../execInApp
            --cflags=-DNO_LOG_CONTROL
            --cflags=-DLEGATO_FRAMEWORK_NICE_LEVEL=${LEGATO_FRAMEWORK_NICE_LEVEL}
            --cflags=-DLE_RUNTIME_DIR='"${LE_RUNTIME_DIR}"'
            --cflags=-DLE_SVCDIR_SERVER_SOCKET_NAME="${LE_SVCDIR_SERVER_SOCKET_NAME}"
            --cflags=-DLE_SVCDIR_CLIENT_SOCKET_NAME="${LE_SVCDIR_CLIENT_SOCKET_NAME}"
            -i ${PROJECT_SOURCE_DIR}/framework/c/src
            -i ${PROJECT_SOURCE_DIR}/interfaces/supervisor
            -i ${PROJECT_SOURCE_DIR}/framework/c/src/serviceDirectory
            -s ${PROJECT_SOURCE_DIR}/framework/c/src
            -v
            ${CFLAGS_TARGET_IMPORTS}
        DEPENDS
            ./*
            Component.cdef
            )
