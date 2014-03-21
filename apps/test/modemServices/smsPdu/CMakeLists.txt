#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_COMPONENT SMSTest)
set(APP_TARGET testModemSmsPdu)

set(APP_SOURCES
    ${APP_SOURCES}
    test_sms_pdu.c
    ${LEGATO_SERVICES_MODEM_DIR}/src/smspdu.c
)

include_directories(${CUNIT_INCLUDE_DIRS})

add_definitions(-DNO_LOG_CONTROL)

set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET}
    ${CUNIT_LIBRARIES}
)

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
