#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)


mkapp(audioCfgTest.adef
    -i ${LEGATO_ROOT}/interfaces/audio
    DEPENDS
        legato
        audioCfgComp/Component.cdef
        audioCfgComp/audioCfgTest.c
)
