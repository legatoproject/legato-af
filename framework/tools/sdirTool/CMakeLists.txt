#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
