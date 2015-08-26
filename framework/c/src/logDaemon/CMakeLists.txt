#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************


mkexe(  logCtrlDaemon
            ../logDaemon
            --cflags=-DNO_LOG_CONTROL
            -i ..
            -i ${LEGATO_ROOT}/interfaces/supervisor
            -i ${LEGATO_ROOT}/interfaces/logDaemon
            -v
        DEPENDS
            Component.cdef
            logDaemon.c
        )
