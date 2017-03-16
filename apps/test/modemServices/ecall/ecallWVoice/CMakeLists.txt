#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

if((LEGATO_COMPONENTS_AUDIO MATCHES "ar7") OR (LEGATO_COMPONENTS_AUDIO MATCHES "ar86"))
    set(CODEC_ENABLE_FLAG 1)
else()
     set(CODEC_ENABLE_FLAG 0)
endif()

# Creates application from the eCallWVoice.adef
mkapp(eCallWVoice.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    --cflags=-DENABLE_CODEC=${CODEC_ENABLE_FLAG}
)

# This is a C test
add_dependencies(tests_c eCallWVoice)
