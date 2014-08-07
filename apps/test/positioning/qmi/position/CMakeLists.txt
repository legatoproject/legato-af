#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_COMPONENT paQmiPosTest)
set(APP_TARGET testPaQmiPos)
set(APP_SOURCES
    paQmiPosTest.c
)

include_directories(
    ${CUNIT_INCLUDE_DIRS}
)

set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET}
    ${LEGATO_COMPONENTS_MODEM_TARGET}
    ${LEGATO_COMPONENTS_GNSS_TARGET}
    ${LEGATO_SERVICES_MODEM_TARGET}
)
