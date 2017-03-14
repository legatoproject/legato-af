#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(APP_TARGET testFwSignalShowStack)

mkexe(${APP_TARGET} -C "-g"
            signalShowStack.c
        )

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/signalShowStack.sh
    ${EXECUTABLE_OUTPUT_PATH}/testFwSignalShowStack.sh
    @ONLY
)

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/testFwSignalShowStack.sh
                       ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
