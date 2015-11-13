#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

# Creates application from the eCallStress.adef
mkapp(eCallStress.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    DEPENDS
        eCallStress/Component.cdef
        eCallStress/eCallStress.c
        legato
)
