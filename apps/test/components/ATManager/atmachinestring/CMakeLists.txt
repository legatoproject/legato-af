#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#*******************************************************************************

set(APP_NAME        "atmachinestring")
set(APP_TARGET      "test${APP_NAME}")
set(APP_SOURCES     "test_${APP_NAME}.c")

add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET}
${CUNIT_LIBRARIES}
${LEGATO_COMPONENTS_AT_MANAGER_TARGET}
pthread
)

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
