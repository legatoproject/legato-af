#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
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
