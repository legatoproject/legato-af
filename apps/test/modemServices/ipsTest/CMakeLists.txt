#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************


find_package(Legato REQUIRED)

mkapp(ipsTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    DEPENDS
        legato
        ipsTest/Component.cdef
        ipsTest/ipsTest.c
)
