#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
