#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_COMPONENT paQmiPosXtraTest)
set(APP_TARGET testPaQmiXtraPos)
set(APP_SOURCES
    paQmiPosXtraTest.c
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

add_custom_command(
    TARGET ${APP_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/xtra.bin ${EXECUTABLE_OUTPUT_PATH}/xtra.bin
)