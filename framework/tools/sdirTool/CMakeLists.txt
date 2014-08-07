#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

mkexe(  sdir
            ../sdirTool
            -i ../../c/src
            -i ../../c/src/serviceDirectory
        DEPENDS
            sdirTool.c
            Component.cdef
        )
