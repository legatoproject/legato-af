#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Use of this work is subject to license.
#*******************************************************************************


find_package(Legato REQUIRED)

mkapp(infoTest.adef
     -i ${LEGATO_ROOT}/interfaces/modemServices
     DEPENDS
        infoTest/Component.cdef
        infoTest/infoTest.c
        legato
)
