#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

# Creates application from the eCallTest.adef
mkapp(eCallTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_ROOT}/components/cfgEntries
    DEPENDS
        eCallTest/Component.cdef
        eCallTest/le_ecallTest.c
        legato
)
