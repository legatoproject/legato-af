#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(LEGATO_INCLUDE_DIRS)
set(LEGATO_INCLUDE_DIRS_PRIV) #TODO: Handle "Private" option

# Build targets & defines
set(LEGATO_FRAMEWORK_TARGET                 legato)
set(LEGATO_FRAMEWORK_DIR                    ${LEGATO_SOURCE_DIR}/framework/c)

set(LEGATO_TREE_HDLR_TARGET                 le_tree_hdlr)
set(LEGATO_TREE_HDLR_DIR                    ${LEGATO_SOURCE_DIR}/components/legatoTreeHandler/implementation)

set(LEGATO_CONFIGTREE_API_TARGET            le_cfg_api)
set(LEGATO_CONFIGTREE_API_DIR               ${LEGATO_SOURCE_DIR}/framework/c/src/configApi)

set(LEGATO_CONFIGTREE_ENTRIES_DIR           ${LEGATO_SOURCE_DIR}/components/cfgEntries)

set(LEGATO_SERVICES_MODEM_TARGET            le_mdm_services)
set(LEGATO_SERVICES_MODEM_DIR               ${LEGATO_SOURCE_DIR}/components/modemServices/implementation)
set(LEGATO_SERVICES_MODEM_CLIENT_TARGET     le_mdm_client)

set(LEGATO_SERVICES_POSITIONING_TARGET      le_pos_services)
set(LEGATO_SERVICES_POSITIONING_DIR         ${LEGATO_SOURCE_DIR}/components/positioning/implementation)
set(LEGATO_SERVICES_POSITIONING_CLIENT_TARGET     le_pos_client)

set(LEGATO_SERVICES_DCS_CLIENT_TARGET       le_data_client)

set(LEGATO_SERVICES_CNET_CLIENT_TARGET      le_cellnet_client)

set(LEGATO_COMPONENTS_MODEM_TARGET          le_pa)
set(LEGATO_COMPONENTS_MODEM_DIR             ${LEGATO_SOURCE_DIR}/components/modemServices/platformAdaptor)
set(LEGATO_COMPONENTS_MODEM_AT_SRC_DIR      ${LEGATO_COMPONENTS_MODEM_DIR}/at/implementation/src)

set(LEGATO_COMPONENTS_AT_MANAGER_TARGET     le_mdm_atmgr)
set(LEGATO_COMPONENTS_AT_MANAGER_DIR        ${LEGATO_SOURCE_DIR}/components/modemServices/platformAdaptor/at/atManager)
set(LEGATO_COMPONENTS_AT_MANAGER_INCLUDE_DIRS
    ${LEGATO_COMPONENTS_AT_MANAGER_DIR}/inc
    ${LEGATO_COMPONENTS_AT_MANAGER_DIR}/devices/adapter_layer/inc
    ${LEGATO_COMPONENTS_AT_MANAGER_DIR}/devices/uart/inc
)

set(LEGATO_SERVICES_AUDIO_TARGET            le_audio_services)
set(LEGATO_SERVICES_AUDIO_DIR               ${LEGATO_SOURCE_DIR}/components/audio/implementation)
set(LEGATO_SERVICES_AUDIO_CLIENT_TARGET     le_audio_client)

set(LEGATO_COMPONENTS_AUDIO_TARGET          le_pa_audio)
set(LEGATO_COMPONENTS_AUDIO_DIR             ${LEGATO_SOURCE_DIR}/components/audio/platformAdaptor)

set(LEGATO_UART_TARGET                      le_uart)
set(LEGATO_UART_TARGET_DIR                  ${LEGATO_COMPONENTS_AT_MANAGER_DIR}/devices/uart)

set(LEGATO_COMPONENTS_GNSS_TARGET           le_pa_gnss)
set(LEGATO_COMPONENTS_GNSS_DIR              ${LEGATO_SOURCE_DIR}/components/positioning/platformAdaptor)

# Tools
set(LEGATO_TOOL_IFGEN                       ${LEGATO_SOURCE_DIR}/bin/ifgen)
set(LEGATO_TOOL_MKAPP                       ${LEGATO_SOURCE_DIR}/bin/mkapp)
set(LEGATO_TOOL_MKEXE                       ${LEGATO_SOURCE_DIR}/bin/mkexe)

# AirVantage
if(INCLUDE_AIRVANTAGE)
    set(AIRVANTAGE_INCLUDE_DIR              ${LEGATO_BINARY_DIR}/airvantage/runtime/itf)
    set(AIRVANTAGE_LIBRARY_DIR              ${LEGATO_BINARY_DIR}/airvantage/runtime/lib)
endif()

# C Framework
set(LEGATO_INCLUDE_DIRS ${LEGATO_INCLUDE_DIRS}
    ${LEGATO_FRAMEWORK_DIR}/inc
)

# C Framework: Private
set(LEGATO_INCLUDE_DIRS_PRIV ${LEGATO_INCLUDE_DIRS_PRIV}
    ${LEGATO_FRAMEWORK_DIR}/src
    ${LEGATO_FRAMEWORK_DIR}/src/logDaemon
)

# High-Level Interfaces
set(LEGATO_INCLUDE_DIRS ${LEGATO_INCLUDE_DIRS}
    ${LEGATO_SOURCE_DIR}/interfaces/legatoTreeHandler/c
    ${LEGATO_SOURCE_DIR}/interfaces/modemServices/c
    ${LEGATO_SOURCE_DIR}/interfaces/positioning/c
    ${LEGATO_SOURCE_DIR}/interfaces/audio/c
    ${LEGATO_SOURCE_DIR}/interfaces/dataConnectionService/c
    ${LEGATO_SOURCE_DIR}/interfaces/cellNetService/c
    ${LEGATO_SOURCE_DIR}/interfaces/config/c
)

# Low-Level Interfaces
set(LEGATO_INCLUDE_DIRS_PRIV ${LEGATO_INCLUDE_DIRS_PRIV}

    # Components
    ${LEGATO_COMPONENTS_MODEM_DIR}/interfaces/inc
    ${LEGATO_COMPONENTS_AT_MANAGER_INCLUDE_DIRS}
    ${LEGATO_COMPONENTS_GNSS_DIR}/interfaces/inc
    ${LEGATO_COMPONENTS_AUDIO_DIR}/interfaces/inc

    # Devices
    ${LEGATO_UART_TARGET_DIR}/inc

    # Apps
    ${LEGATO_SERVICES_AUDIO_DIR}/src
    ${LEGATO_SERVICES_MODEM_DIR}/src
    ${LEGATO_SERVICES_POSITIONING_DIR}/src

    # ConfigDB entries
    ${LEGATO_CONFIGTREE_ENTRIES_DIR}
)

set(LEGATO_LIBRARIES
    ${LEGATO_FRAMEWORK_TARGET}
)

# Function to add a compile flag for a given target.
# Put the flags in quotes.
# e.g., add_compile_flags(myExe "-DDEBUG")
function(add_compile_flags COMPILE_TARGET)

    set_target_properties(${COMPILE_TARGET} PROPERTIES COMPILE_FLAGS ${ARGN})

endfunction()

function(set_legato_component APP_COMPONENT)
    # Log Configuration
    add_definitions(-DLE_COMPONENT_NAME=${APP_COMPONENT})
    add_definitions(-DLE_LOG_SESSION=${APP_COMPONENT}_LogSession)
    add_definitions(-DLE_LOG_LEVEL_FILTER_PTR=${APP_COMPONENT}_LogLevelFilterPtr)
endfunction()

function(clear_legato_component APP_COMPONENT)
    remove_definitions(-DLE_COMPONENT_NAME=${APP_COMPONENT})
    remove_definitions(-DLE_LOG_SESSION=${APP_COMPONENT}_LogSession)
    remove_definitions(-DLE_LOG_LEVEL_FILTER_PTR=${APP_COMPONENT}_LogLevelFilterPtr)
endfunction()

# Function to compile applications
function(add_legato_executable EXE_TARGET)

    # Other arguments are sources
    set(EXE_SOURCES ${ARGN})

    # Legato Includes
    include_directories(${LEGATO_INCLUDE_DIRS})

    # Compile
    add_executable(${EXE_TARGET}
        ${LEGATO_SOURCE_DIR}/framework/c/codegen/_le_main.c
        ${EXE_SOURCES}
    )

    # Link
    target_link_libraries(${EXE_TARGET}
        ${LEGATO_LIBRARIES}
    )

    # Dependencies
    add_dependencies(${EXE_TARGET} ${LEGATO_FRAMEWORK_TARGET})

    # Compile flags
    add_compile_flags(${EXE_TARGET} "-DLE_EXECUTABLE_NAME=${EXE_TARGET}")

endfunction()

# Function to compile internal applications
function(add_legato_internal_executable)

    # Legato Private Includes
    include_directories(${LEGATO_INCLUDE_DIRS_PRIV})

    add_legato_executable(${ARGN})

endfunction()


# Function to build a Legato executable using mkexe.
# The executable will be put in the appropriate target's bin directory.
# Supporting libraries will be put in the target's lib directory.
# The first parameter to the function must be the name of the exe file.
# Any subsequent parameters will be passed as-is to mkexe on its command line.
function(mkexe EXE_NAME)

    add_custom_command(
            OUTPUT ${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME}
            COMMAND ${LEGATO_TOOL_MKEXE}
                          -o ${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME}
                          -w ${CMAKE_CURRENT_BINARY_DIR}
                          -l ${LIBRARY_OUTPUT_PATH}
                          -t ${LEGATO_TARGET}
                          -i ${PROJECT_SOURCE_DIR}/framework/c/inc
                          -c ${CMAKE_CURRENT_BINARY_DIR}
                          ${ARGN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "mkexe '${EXE_NAME}': ${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME}"
    )

    add_custom_target(${EXE_NAME}
            ALL
            DEPENDS ${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME}
    )

endfunction()

# Function to build a Legato application using mkapp
# Any subsequent parameters will be passed as-is to mkapp on its command line.
function(mkapp APP_NAME ADEF)

    get_filename_component(APP_FILE ${ADEF} NAME_WE)
    set(APP_PKG "${EXECUTABLE_OUTPUT_PATH}/${APP_FILE}.${LEGATO_TARGET}")

    add_custom_command(
            OUTPUT ${APP_PKG}
            COMMAND
                PATH=${LEGATO_SOURCE_DIR}/bin:$ENV{PATH}
                ${LEGATO_TOOL_MKAPP}
                        ${ADEF}
                        -t ${LEGATO_TARGET}
                        -w ${CMAKE_CURRENT_BINARY_DIR}
                        -i ${CMAKE_CURRENT_SOURCE_DIR}
                        -c ${CMAKE_CURRENT_SOURCE_DIR}
                        -o ${EXECUTABLE_OUTPUT_PATH}
                        -v
                        ${ARGN}
            COMMAND
            DEPENDS ${ADEF}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "mkapp '${APP_NAME}': ${APP_PKG}"
    )

    add_custom_target(${APP_NAME}
            ALL
            DEPENDS ${APP_PKG}
    )

endfunction()
