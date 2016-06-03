#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

if(LEGATO_COMPONENTS_AUDIO EQUAL "ar7")
    set(CODEC_ENABLE_FLAG 1)
else()
     set(CODEC_ENABLE_FLAG 0)
endif()


mkapp(audioPlaybackRec.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    --cflags=-DENABLE_CODEC=${CODEC_ENABLE_FLAG}
)
