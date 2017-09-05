#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(TEST_EXEC cellNetUnitTest)

set(LEGATO_CELLNET "${LEGATO_ROOT}/components/cellNetService/")
set(LEGATO_MODEM_SERVICES "${LEGATO_ROOT}/components/modemServices/")

if(TEST_COVERAGE EQUAL 1)
    set(CFLAGS "--cflags=\"--coverage\"")
    set(LFLAGS "--ldflags=\"--coverage\"")
endif()

mkexe(${TEST_EXEC}
    cellNetComp
    .
    -i ${LEGATO_CELLNET}/
    -i ${LEGATO_ROOT}/framework/liblegato
    -i ${LEGATO_ROOT}/interfaces/modemServices
    ${CFLAGS}
    ${LFLAGS}
    -C "-fvisibility=default"
)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC})

# This is a C test
add_dependencies(tests_c ${TEST_EXEC})
