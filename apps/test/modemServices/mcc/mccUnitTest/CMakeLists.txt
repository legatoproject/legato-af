#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(TEST_EXEC mccUnitTest)

set(LEGATO_MODEM_SERVICES "${LEGATO_ROOT}/components/modemServices/")

if(TEST_COVERAGE EQUAL 1)
    set(CFLAGS "--cflags=\"--coverage\"")
    set(LFLAGS "--ldflags=\"--coverage\"")
endif()

mkexe(${TEST_EXEC}
    .
    -i ${LEGATO_MODEM_SERVICES}/modemDaemon
    -i ${LEGATO_MODEM_SERVICES}/platformAdaptor/inc
    -i ${LEGATO_ROOT}/framework/liblegato
    -i ${PA_DIR}/simu/components/le_pa
    -s ${PA_DIR}
    ${CFLAGS}
    ${LFLAGS}
)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC})

# This is a C test
add_dependencies(tests_c ${TEST_EXEC})
