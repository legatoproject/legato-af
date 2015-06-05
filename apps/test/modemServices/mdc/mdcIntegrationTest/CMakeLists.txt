#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

mkapp(mdcTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    DEPENDS
        legato
        mdcTestComp/Component.cdef
        mdcTestComp/mdcTest.c
)
