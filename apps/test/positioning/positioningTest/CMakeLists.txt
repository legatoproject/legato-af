#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)
#find_package(CUnit REQUIRED)

# Creates application from the positioningTest.adef
mkapp(positioningTest.adef
    -i ${LEGATO_ROOT}/interfaces/positioning
    DEPENDS
        positioningTest/Component.cdef
        positioningTest/positioningTest.c
        legato
)
