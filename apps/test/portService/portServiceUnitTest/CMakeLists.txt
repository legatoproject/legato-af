#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(TEST_EXEC portServiceUnitTest)

set(LEGATO_PORT_SERVICE "${LEGATO_ROOT}/components/portService")
set(LEGATO_AT_SERVICES "${LEGATO_ROOT}/components/atServices")
set(PORTSERVICEJSONFILE "${LEGATO_ROOT}/apps/test/portService/portServiceUnitTest/PortConfigurationFile.json")

if(TEST_COVERAGE EQUAL 1)
    set(CFLAGS "--cflags=\"--coverage\"")
    set(LFLAGS "--ldflags=\"--coverage\"")
endif()

mkexe(${TEST_EXEC}
    portServiceComp
    .
    -i ${LEGATO_PORT_SERVICE}/portDaemon
    -i ${LEGATO_AT_SERVICES}
    -i ${LEGATO_AT_SERVICES}/Common
    -i ${LEGATO_ROOT}/framework/liblegato/linux/
    -i ${LEGATO_ROOT}/interfaces/atServices/
    -i ${LEGATO_ROOT}/interfaces/portService/
    -i ${LEGATO_ROOT}/interfaces/
    -i ${LEGATO_ROOT}/components/watchdogChain/
    ${CFLAGS}
    ${LFLAGS}
    -C "-fvisibility=default -g"
)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC} ${PORTSERVICEJSONFILE})

# This is a C test
add_dependencies(tests_c ${TEST_EXEC})
