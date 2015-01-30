#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_TARGET testAudioServiceMemory)

if(${LEGATO_TARGET} STREQUAL "ar7")
    mkexe(${APP_TARGET}
                .
                -i ${LEGATO_ROOT}/interfaces/audio
            DEPENDS
                legato
                ## TODO: Remove this when the mk tools do dependency checking.
                Component.cdef
                audioMemoryTest.c
                )

    add_test(${MEM_APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${MEM_APP_TARGET} 1)
    set_tests_properties(${MEM_APP_TARGET} PROPERTIES TIMEOUT 0)

endif()
