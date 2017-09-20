#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

if ($ENV{TARGET} MATCHES "localhost")
    set(LEGATO_FRAMEWORK_SRC "${LEGATO_ROOT}/framework/liblegato")
    set(LEGATO_FRAMEWORK_INC "${LEGATO_ROOT}/framework/include")
    set(LEGATO_POS_SERVICES "${LEGATO_ROOT}/components/positioning/posDaemon")
    set(LEGATO_POS_PA "${LEGATO_ROOT}/components/positioning/platformAdaptor")
    set(LEGATO_CFG_ENTRIES "${LEGATO_ROOT}/components/cfgEntries")
    set(LEGATO_CFG_TREE "${LEGATO_FRAMEWORK_SRC}/configTree")

    set(TEST_BIN positioningUnitTest)
    set(TEST_SOURCE "${LEGATO_ROOT}/apps/test/positioning/positioningUnitTest")

    set(MKEXE_CFLAGS "-fvisibility=default -g $ENV{CFLAGS}")

    if(TEST_COVERAGE EQUAL 1)
        set(CFLAGS "--cflags=\"--coverage\"")
        set(LFLAGS "--ldflags=\"--coverage\"")
    endif()

    mkexe(${TEST_BIN}
        pos
        .
        -i pos
        -i pos/gnss
        -i ${LEGATO_FRAMEWORK_SRC}
        -i ${LEGATO_FRAMEWORK_INC}
        -i ${LEGATO_CFG_TREE}
        -i ${LEGATO_POS_SERVICES}
        -i ${LEGATO_POS_PA}/inc
        -i ${LEGATO_CFG_ENTRIES}
        ${CFLAGS}
        ${LFLAGS}
        -C ${MKEXE_CFLAGS}
    )

    add_test(${TEST_BIN} ${EXECUTABLE_OUTPUT_PATH}/${TEST_BIN})

    # This is a C test
    add_dependencies(tests_c ${TEST_BIN})
endif()
