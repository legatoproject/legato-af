#**********************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#**********************************************************************************

mkcomp( ../security
            -i ..
            -i ../serviceDirectory
            --cflags=-DLEGATO_FRAMEWORK_NICE_LEVEL=${LEGATO_FRAMEWORK_NICE_LEVEL}
            --cflags=-DLE_RUNTIME_DIR='"${LE_RUNTIME_DIR}"'
            --cflags=-DLE_SVCDIR_SERVER_SOCKET_NAME="${LE_SVCDIR_SERVER_SOCKET_NAME}"
            --cflags=-DLE_SVCDIR_CLIENT_SOCKET_NAME="${LE_SVCDIR_CLIENT_SOCKET_NAME}"
        DEPENDS
            resourceLimits.c
            sandbox.c
            cgroups.c
            app.c
            proc.c
            watchdogAction.c
            smack.c
            ${LEGATO_ROOT}/interfaces/le_cfg.api
        )
