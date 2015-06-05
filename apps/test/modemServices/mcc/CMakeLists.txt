#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

mkapp(mccTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_ROOT}/interfaces/audio
    DEPENDS
        legato
        mccTest/Component.cdef
        mccTest/le_mccTest.c
        ${LEGATO_ROOT}/interfaces/modemServices/le_mdmDefs.api
)
