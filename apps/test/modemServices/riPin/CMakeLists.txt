#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************


find_package(Legato REQUIRED)

mkapp(riPinTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    DEPENDS
        legato
        riPinTest/Component.cdef
        riPinTest/riPinTest.c
)
