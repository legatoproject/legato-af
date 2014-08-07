#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_COMPONENT paQmiVoiceTest)
set(APP_TARGET testPaQmiVoice)
set(APP_SOURCES
    paQmiVoiceTest.c
)

set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET}
    ${LEGATO_COMPONENTS_MODEM_TARGET}
    ${LEGATO_SERVICES_MODEM_TARGET}
)
