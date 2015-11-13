#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************


find_package(Legato REQUIRED)

if ( (LEGATO_TARGET MATCHES "ar7") OR (LEGATO_TARGET MATCHES "ar86") )
    mkapp(voicePromptMcc.adef
        -i ${LEGATO_ROOT}/interfaces/modemServices
        -i ${LEGATO_ROOT}/interfaces/audio
        DEPENDS
            legato
            voicePromptMccComp/Component.cdef
            voicePromptMccComp/voicePromptMcc.c
    )
endif()
