#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Use of this work is subject to license.
#*******************************************************************************


find_package(Legato REQUIRED)

mkapp( voiceCallTest.adef
       -i ${LEGATO_ROOT}/interfaces/modemServices
       -i ${LEGATO_ROOT}/interfaces/voiceCallService
       -i ${LEGATO_ROOT}/interfaces/audio
)
