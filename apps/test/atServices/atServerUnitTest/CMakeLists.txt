#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
    -i ${LEGATO_ROOT}/framework/c/src
    -i ${LEGATO_AT_SERVER}/Common
    ${CFLAGS}
    ${LFLAGS}
    -C "-fvisibility=default -g"
)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC})
