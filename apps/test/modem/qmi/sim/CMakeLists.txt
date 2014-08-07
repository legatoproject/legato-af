#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_COMPONENT pa_qmi_sim_Test)
set(APP_TARGET testPaQmiSim)
set(APP_SOURCES
    pa_qmi_sim_Test.c
)

set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET}
    ${LEGATO_COMPONENTS_MODEM_TARGET}
    ${LEGATO_SERVICES_MODEM_TARGET}
)
