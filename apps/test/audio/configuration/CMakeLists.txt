#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_TARGET testAudioCfg)

mkexe(${APP_TARGET}
            cfgTest
            -i ${LEGATO_ROOT}/interfaces/audio
        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            cfgTest/Component.cdef
            cfgTest/audioCfgTest.c
            )

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()