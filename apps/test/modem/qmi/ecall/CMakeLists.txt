#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_COMPONENT paQmiECallTest)
set(APP_TARGET testPaQmiECall)
set(APP_SOURCES
    paQmiECallTest.c
)

include_directories(
    ${LEGATO_COMPONENTS_MODEM_DIR}/qmi/src
    ${LEGATO_SOURCE_DIR}/proprietary/qct/inc/framework
    ${LEGATO_SOURCE_DIR}/proprietary/qct/inc/services
)

set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET}
    ${LEGATO_COMPONENTS_MODEM_TARGET}
)
