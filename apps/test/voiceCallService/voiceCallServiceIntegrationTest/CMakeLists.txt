#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
#
#*******************************************************************************


find_package(Legato REQUIRED)

mkapp( voiceCallTest.adef
       -i ${LEGATO_ROOT}/interfaces/modemServices
       -i ${LEGATO_ROOT}/interfaces/voiceCallService
       -i ${LEGATO_ROOT}/interfaces/audio
)

# This is a C test
add_dependencies(tests_c voiceCallTest)
