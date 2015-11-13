#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

mkapp( simProfileSwap.adef
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/interfaces/audio
            -i ${LEGATO_ROOT}/interfaces/modemServices
        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            simTestComp/Component.cdef
            simTestComp/simTest.c
)
