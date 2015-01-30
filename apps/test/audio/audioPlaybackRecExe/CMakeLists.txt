#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_TARGET testAudioPlaybackRecExe)

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()

# AR7 Component Implementation
if(LEGATO_COMPONENTS_AUDIO MATCHES "ar7")
    set(CODEC_ENABLE_FLAG 1)
else()
    set(CODEC_ENABLE_FLAG 0)
endif()

mkexe(  ${APP_TARGET}
            .
            -i ${LEGATO_ROOT}/interfaces/audio
            -i ${LEGATO_ROOT}/interfaces/modemServices
            --cflags=-DENABLE_CODEC=${CODEC_ENABLE_FLAG}
            DEPENDS
            legato
            Component.cdef
            audioPlaybackRecExe.c
    )
