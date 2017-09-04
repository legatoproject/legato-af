#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(LEGATO_SMSINBOXSVC "${LEGATO_ROOT}/components/smsInboxService/")
set(LEGATO_MODEM_SERVICES "${LEGATO_ROOT}/components/modemServices/")
set(JANSSON_INC_DIR "${CMAKE_BINARY_DIR}/framework/libjansson/include/")

set(SMSINBOXCFGFILE1 "${LEGATO_ROOT}/apps/test/smsInboxService/smsInboxServiceUnitTest/smsInboxConfigFiles/cfg/le_smsInbox1.json")
set(SMSINBOXCFGFILE2 "${LEGATO_ROOT}/apps/test/smsInboxService/smsInboxServiceUnitTest/smsInboxConfigFiles/cfg/le_smsInbox2.json")
set(SMSINBOXMSGFILE1 "${LEGATO_ROOT}/apps/test/smsInboxService/smsInboxServiceUnitTest/smsInboxConfigFiles/msg/0000002d.json")
set(SMSINBOXMSGFILE2 "${LEGATO_ROOT}/apps/test/smsInboxService/smsInboxServiceUnitTest/smsInboxConfigFiles/msg/0000002e.json")
set(SMSINBOXMSGFILE3 "${LEGATO_ROOT}/apps/test/smsInboxService/smsInboxServiceUnitTest/smsInboxConfigFiles/msg/0000002f.json")

set(TEST_EXEC smsInboxServiceUnitTest)
set(MKEXE_CFLAGS "-fvisibility=default -g $ENV{CFLAGS}")

if(TEST_COVERAGE EQUAL 1)
    set(CFLAGS "--cflags=\"--coverage\"")
    set(LFLAGS "--ldflags=\"--coverage\"")
endif()

mkexe(${TEST_EXEC}
    smsInboxServiceComp
    .
    -i ${LEGATO_SMSINBOXSVC}
    -i smsInboxServiceComp
    -i ${LEGATO_MODEM_SERVICES}
    -i ${LEGATO_ROOT}/framework/liblegato/
    -i ${LEGATO_ROOT}/interfaces/modemServices/
    -i ${LEGATO_ROOT}/interfaces/
    -i ${JANSSON_INC_DIR}
    ${CFLAGS}
    ${LFLAGS}
    -C ${MKEXE_CFLAGS}
    -L "-ljansson"
)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC} ${SMSINBOXCFGFILE1} ${SMSINBOXCFGFILE2} ${SMSINBOXMSGFILE1} ${SMSINBOXMSGFILE2} ${SMSINBOXMSGFILE3})

# This is a C test
add_dependencies(tests_c ${TEST_EXEC})

