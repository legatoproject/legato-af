#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

include_directories(
    ${LEGATO_INCLUDE_DIRS}
    ${LEGATO_INCLUDE_DIRS_PRIV}
)

add_library(${LEGATO_SERVICES_POSITIONING_TARGET} SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/le_pos.c
)

target_link_libraries(${LEGATO_SERVICES_POSITIONING_TARGET}
                      ${LEGATO_COMPONENTS_GNSS_TARGET}
                      ${LEGATO_FRAMEWORK_TARGET}
                      ${LEGATO_CONFIGTREE_API_TARGET}
                      m
)
