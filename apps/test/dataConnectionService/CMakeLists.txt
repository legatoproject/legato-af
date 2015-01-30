#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

mkapp(dcsTest.adef
    -i ${LEGATO_ROOT}/interfaces/dataConnectionService
    DEPENDS
        legato
        dcsTestComp/Component.cdef
        dcsTestComp/dcsTest.c
)


