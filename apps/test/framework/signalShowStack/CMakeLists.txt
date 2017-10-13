#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(APP_TARGET testFwSignalShowStack)

# As the source "is doing bad things" in the aim to produce error generating
# signals (SEGV, ILL, BUS, FPE, ABRT), some compilers detect these and do not
# compile as warning are raised. So add the -Wno-error option specifically
# for this test source.
mkexe(${APP_TARGET} -C "-O2 -fno-omit-frame-pointer -Wno-error"
            signalShowStack.c
        )

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/signalShowStack.sh
    ${EXECUTABLE_OUTPUT_PATH}/testFwSignalShowStack.sh
    @ONLY
)

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/testFwSignalShowStack.sh
                       ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})

# This is a C test
add_dependencies(tests_c ${APP_TARGET})
