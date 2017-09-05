#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(LEGATO_VOICECALL "${LEGATO_ROOT}/components/voiceCallService/")
set(LEGATO_MODEM_SERVICES "${LEGATO_ROOT}/components/modemServices/")

set(TEST_EXEC voiceCallServiceUnitTest)
set(MKEXE_CFLAGS "-fvisibility=default -g $ENV{CFLAGS}")

if(TEST_COVERAGE EQUAL 1)
    set(CFLAGS "--cflags=\"--coverage\"")
    set(LFLAGS "--ldflags=\"--coverage\"")
endif()

mkexe(${TEST_EXEC}
    voiceCallServiceComp
    .
    -i ${LEGATO_VOICECALL}
    -i voiceCallServiceComp
    -i ${LEGATO_MODEM_SERVICES}
    -i ${LEGATO_ROOT}/framework/liblegato
    -i ${LEGATO_ROOT}/interfaces/modemServices/
    -i ${LEGATO_ROOT}/interfaces/
    ${CFLAGS}
    ${LFLAGS}
    -C ${MKEXE_CFLAGS}
)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC})

# This is a C test
add_dependencies(tests_c ${TEST_EXEC})
