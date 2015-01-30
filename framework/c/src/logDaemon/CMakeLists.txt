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
            -v
        DEPENDS
            Component.cdef
            logDaemon.c
        )
