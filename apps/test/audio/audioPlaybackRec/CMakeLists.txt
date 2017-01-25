#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

if( (LEGATO_COMPONENTS_AUDIO STREQUAL "ar7") OR (LEGATO_COMPONENTS_AUDIO STREQUAL "ar758x") )
    set(CODEC_ENABLE_FLAG 1)
else()
     set(CODEC_ENABLE_FLAG 0)
endif()


mkapp(audioPlaybackRec.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    --cflags=-DENABLE_CODEC=${CODEC_ENABLE_FLAG}
)
