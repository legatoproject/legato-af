#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

project(LOGTOOL C)

find_package(Legato REQUIRED)

mkexe(  log
            logTool.c
            -i ../../c/src
            -i ../../c/src/logDaemon
        DEPENDS
            logTool.c
        )
