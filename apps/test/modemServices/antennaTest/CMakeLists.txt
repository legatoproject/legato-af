#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

mkapp(antennaTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    DEPENDS
        legato
        monAntennaTestComp/Component.cdef
        monAntennaTestComp/monAntennaTest.c
)
