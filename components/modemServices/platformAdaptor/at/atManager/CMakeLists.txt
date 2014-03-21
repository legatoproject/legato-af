#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

project(LEGATO_ATMGR C)

find_package(Legato REQUIRED)

if(LEGATO_COMPONENTS_MODEM MATCHES "AT")

    set_legato_component(atmanager)

    # Includes
    include_directories(
        ${LEGATO_INCLUDE_DIRS}
        ${LEGATO_INCLUDE_DIRS_PRIV}
        ${LEGATO_COMPONENTS_AT_MANAGER_INCLUDE_DIRS}
    )

    # Sources
    aux_source_directory(src LEGATO_ATMGR_SRC)

    # Builds library
    add_library(${LEGATO_COMPONENTS_AT_MANAGER_TARGET} SHARED
        ${LEGATO_ATMGR_SRC}
    )

    # Linking
    target_link_libraries(${LEGATO_COMPONENTS_AT_MANAGER_TARGET}
        ${LEGATO_FRAMEWORK_TARGET}
        ${LEGATO_UART_TARGET}
    )

endif()
