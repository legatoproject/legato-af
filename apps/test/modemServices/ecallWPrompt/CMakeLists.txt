#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

if(LEGATO_COMPONENTS_AUDIO MATCHES "ar7")
    set(CODEC_ENABLE_FLAG 1)
else()
     set(CODEC_ENABLE_FLAG 0)
endif()

# Creates application from the eCallWPrompt.adef
mkapp(eCallWPrompt.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_ROOT}/interfaces/audio
    --cflags=-DENABLE_CODEC=${CODEC_ENABLE_FLAG}
    DEPENDS
        eCallWPrompt/Component.cdef
        eCallWPrompt/eCallWPrompt.c
        legato
)
