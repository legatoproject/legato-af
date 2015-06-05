#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

# Creates application from the gnssTest.adef
mkapp(gnssTest.adef
    -i ${LEGATO_ROOT}/interfaces/positioning
    DEPENDS
        gnssTest/Component.cdef
        gnssTest/gnssTest.c
        legato
)
