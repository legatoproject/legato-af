# --------------------------------------------------------------------------------------------------
# Copyright (C) 2014, Sierra Wireless Inc., Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

# Don't try to connect to the Log Control Daemon.
add_definitions()

# Build the Service Directory.
mkexe(  serviceDirectory
            serviceDirectory.c
            -i ..
            -v
            --cflags=-DNO_LOG_CONTROL
            --cflags=-DLE_RUNTIME_DIR="${LE_RUNTIME_DIR}"
            --cflags=-DLE_SVCDIR_SERVER_SOCKET_NAME="${LE_SVCDIR_SERVER_SOCKET_NAME}"
            --cflags=-DLE_SVCDIR_CLIENT_SOCKET_NAME="${LE_SVCDIR_CLIENT_SOCKET_NAME}"
        DEPENDS
            serviceDirectory.c
            serviceDirectoryProtocol.h
        )
