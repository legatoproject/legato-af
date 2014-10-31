#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)


mkapp(  voiceCallService.adef
            -i ${LEGATO_ROOT}/interfaces/voiceCallService
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/interfaces/audio
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_ROOT}/components/voiceCallService/Component.cdef
            ${LEGATO_ROOT}/components/voiceCallService/voiceCallServiceServer.c
            ${LEGATO_ROOT}/interfaces/voiceCallService/le_voicecall.api
            ${LEGATO_ROOT}/interfaces/audio/le_audio.api
            ${LEGATO_ROOT}/interfaces/modemServices/le_mcc_call.api
            ${LEGATO_ROOT}/interfaces/modemServices/le_mcc_profile.api
            ${LEGATO_ROOT}/interfaces/modemServices/le_mdmDefs.api
        )

