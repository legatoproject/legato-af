#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)


mkapp(cfgTest.adef
    -i ${LEGATO_ROOT}/interfaces/audio
    DEPENDS
        legato
        cfgTestComp/Component.cdef
        cfgTestComp/audioCfgTest.c
)
