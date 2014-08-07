#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

mkexe(  appCtrl
            ../appCtrl
            -i "${PROJECT_SOURCE_DIR}/interfaces/supervisor"
            -i ../../c/src
            -v
        DEPENDS
            appCtrl.c
            Component.cdef
        )
