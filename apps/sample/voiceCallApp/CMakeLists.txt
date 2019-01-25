#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
#
#*******************************************************************************


find_package(Legato REQUIRED)

mkapp( voiceCallApp.adef
       -i ${LEGATO_ROOT}/interfaces/modemServices
       -i ${LEGATO_ROOT}/interfaces/voiceCallService
       -i ${LEGATO_ROOT}/interfaces/audio
)
