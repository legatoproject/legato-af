#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(TEST_EXEC fwupdateUnitTest)
set(LEGATO_FWUPDATE "${LEGATO_ROOT}/components/fwupdate")
set(FILEPATH "${LEGATO_ROOT}/apps/test/fwupdate/files")

if(TEST_COVERAGE EQUAL 1)
    set(CFLAGS "--cflags=\"--coverage\"")
    set(LFLAGS "--ldflags=\"--coverage\"")
endif()


mkexe(${TEST_EXEC}
    ${PA_DIR}/simu/components/le_pa_fwupdate
    fwupdateComp
    .
    -i ${LEGATO_FWUPDATE}/fwupdateDaemon
    -i ${LEGATO_ROOT}/framework/liblegato
    -i ${LEGATO_FWUPDATE}/platformAdaptor/inc
    -i ${LEGATO_ROOT}/platformAdaptor/simu/components/le_pa_fwupdate
    -i ${PA_DIR}/simu/components/le_pa_fwupdate
    -i ${LEGATO_ROOT}/
    -C "-fvisibility=default"
    ${CFLAGS}
    ${LFLAGS}
    -C "-g"
)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC}  ${FILEPATH} )

# This is a C test
add_dependencies(tests_c ${TEST_EXEC})
