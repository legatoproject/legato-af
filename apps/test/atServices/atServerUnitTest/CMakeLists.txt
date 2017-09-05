#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(TEST_EXEC atServerUnitTest)

set(LEGATO_AT_SERVICES "${LEGATO_ROOT}/components/atServices")

if(TEST_COVERAGE EQUAL 1)
    set(CFLAGS "--cflags=\"--coverage\"")
    set(LFLAGS "--ldflags=\"--coverage\"")
endif()

mkexe(${TEST_EXEC}
    atServerComp
    .
    -i ${LEGATO_ROOT}/framework/liblegato
    -i ${LEGATO_AT_SERVICES}/Common
    ${CFLAGS}
    ${LFLAGS}
    -C "-fvisibility=default -g"
)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC})

# This is a C test
add_dependencies(tests_c ${TEST_EXEC})
