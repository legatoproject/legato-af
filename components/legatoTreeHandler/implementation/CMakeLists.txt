#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set_legato_component(legatoTreeHdlr)

include_directories(
    ${LEGATO_INCLUDE_DIRS}
    ${LEGATO_INCLUDE_DIRS_PRIV}
    # temporary adding until return codes are not converged
    ${LEGATO_SOURCE_DIR}/airvantage/libs/common
    ${LEGATO_SOURCE_DIR}/airvantage/agent/agent/treemgr/handlers/extvars
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src LEGATO_TREE_HDLR_SRC)

add_library(${LEGATO_TREE_HDLR_TARGET} SHARED
    ${LEGATO_TREE_HDLR_SRC}
)

target_link_libraries(${LEGATO_TREE_HDLR_TARGET}
    ${LEGATO_FRAMEWORK_TARGET}
    ${LEGATO_SERVICES_MODEM_CLIENT_TARGET}
    ${LEGATO_SERVICES_POSITIONING_CLIENT_TARGET}
    ${LEGATO_SERVICES_POSSER_CLIENT_TARGET}
)

