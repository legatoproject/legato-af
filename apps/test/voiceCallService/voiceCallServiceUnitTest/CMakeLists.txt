#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(LEGATO_VOICECALL "${LEGATO_ROOT}/components/voiceCallService/")
set(LEGATO_MODEM_SERVICES "${LEGATO_ROOT}/components/modemServices/")
set(LEGATO_AUDIO "${LEGATO_ROOT}/components/audio/")
set(LEGATO_FRAMEWORK_SRC "${LEGATO_ROOT}/framework/c/src")
set(LEGATO_FRAMEWORK_INC "${LEGATO_ROOT}/framework/c/inc")
set(LEGATO_CFG_ENTRIES "${LEGATO_ROOT}/components/cfgEntries")
set(LEGATO_CFG_TREE "${LEGATO_FRAMEWORK_SRC}/configTree")

set(TEST_EXEC voiceCallServiceUnitTest)
set(TEST_SOURCE "${LEGATO_ROOT}/apps/test/voiceCallService/voiceCallServiceUnitTest/")

set(MKEXE_CFLAGS "-fvisibility=default -g $ENV{CFLAGS}")

if(TEST_COVERAGE EQUAL 1)
    set(CFLAGS "--cflags=\"--coverage\"")
    set(LFLAGS "--ldflags=\"--coverage\"")
endif()

mkexe(${TEST_EXEC}
    ${TEST_SOURCE}
    ${PA_DIR}/simu/components/le_pa_audio
    -i ${LEGATO_VOICECALL}/
    -i ${LEGATO_FRAMEWORK_SRC}
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_MODEM_SERVICES}/modemDaemon
    -i ${LEGATO_MODEM_SERVICES}/platformAdaptor/inc
    -i ${PA_DIR}/simu/components/le_pa
    -i ${LEGATO_AUDIO}/platformAdaptor/inc
    -i ${PA_DIR}/simu/components/le_pa_audio
    ${CFLAGS}
    ${LFLAGS}
    -C ${MKEXE_CFLAGS}
)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC})

