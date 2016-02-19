#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

find_package(CUnit REQUIRED)

message("CUNIT_INCLUDE_DIRS: ${CUNIT_INCLUDE_DIRS}")

set(APP_NAME        "atmachinestring")
set(APP_TARGET      "test${APP_NAME}")
set(APP_SOURCES     "test_${APP_NAME}.c")

mkexe(${APP_TARGET}
            .
            ${CUNIT_LIBRARIES}
            -i ${CUNIT_INSTALL}/include
            -i ${CUNIT_INSTALL}/include/CUnit
            -i ${LEGATO_ROOT}/components/atManager/inc
            -i ${LEGATO_ROOT}/components/atManager/devices/adapter_layer/inc
            -i ${LEGATO_ROOT}/components/atManager/devices/uart/inc
            -i ${LEGATO_ROOT}/components/atManager/src
            -i ${LEGATO_ROOT}/components
         )

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
