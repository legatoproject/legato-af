#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(TEST_EXEC audioUnitTest)

set(LEGATO_AUDIO "${LEGATO_ROOT}/components/audio/")

if(TEST_COVERAGE EQUAL 1)
    set(CFLAGS "--cflags=\"--coverage\"")
    set(LFLAGS "--ldflags=\"--coverage\"")
endif()

mkexe(${TEST_EXEC}
    ${PA_DIR}/simu/components/le_pa_audio
    audioComp
    .
    -i ${LEGATO_AUDIO}/
    -i ${LEGATO_AUDIO}/platformAdaptor/inc
    -i ${LEGATO_ROOT}/framework/liblegato
    -i ${PA_DIR}/simu/components/le_pa_audio
    -C "-fvisibility=default"
    ${CFLAGS}
    ${LFLAGS}
)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC})

# This is a C test
add_dependencies(tests_c ${TEST_EXEC})
