#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

mkapp(cellNetServiceTest.adef
     -i ${LEGATO_ROOT}/interfaces/cellNetService
     DEPENDS
        cellNetServiceTest/Component.cdef
        cellNetServiceTest/cellNetServiceTest.c
        legato
)
