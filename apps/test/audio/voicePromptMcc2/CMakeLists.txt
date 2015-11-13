#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************


find_package(Legato REQUIRED)

mkapp(voicePromptMcc2.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_ROOT}/interfaces/audio
    DEPENDS
        legato
        voicePromptMcc2Comp/Component.cdef
        voicePromptMcc2Comp/voicePromptMcc2.c
)
