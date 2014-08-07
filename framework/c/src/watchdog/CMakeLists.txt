#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

# Build the watchdog.
mkexe(  watchdog
            ../watchdog
            -i ..
            -i $LEGATO_ROOT/interfaces/supervisor
            --cflags=-DLE_SVCDIR_SERVER_SOCKET_NAME="${LE_SVCDIR_SERVER_SOCKET_NAME}"
            --cflags=-DLE_SVCDIR_CLIENT_SOCKET_NAME="${LE_SVCDIR_CLIENT_SOCKET_NAME}"
            -v
        DEPENDS
            ## TODO: Remove this stuff when the mk tools do dependency checking.
            watchdog.c
            Component.cdef
            ${LEGATO_ROOT}/interfaces/le_wdog.api
            ${LEGATO_ROOT}/interfaces/supervisor/le_sup_wdog.api
    )
