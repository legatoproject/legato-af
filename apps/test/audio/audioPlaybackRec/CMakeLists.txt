#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************


find_package(Legato REQUIRED)

if(LEGATO_COMPONENTS_AUDIO MATCHES "ar7")
    set(CODEC_ENABLE_FLAG 1)
else()
     set(CODEC_ENABLE_FLAG 0)
endif()


mkapp(audioPlaybackRec.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_ROOT}/interfaces/audio
    --cflags=-DENABLE_CODEC=${CODEC_ENABLE_FLAG}
    DEPENDS
        legato
        audioPlaybackRecComp/Component.cdef
        audioPlaybackRecComp/audioPlaybackRec.c
)
