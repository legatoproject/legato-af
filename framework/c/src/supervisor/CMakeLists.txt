#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

project(SUPERVISOR C)

find_package(Legato REQUIRED)

# Create a variable that points to the ifgen tool.
set(IFGEN_TOOL ${LEGATO_SOURCE_DIR}/framework/tools/ifgen/ifgen)

# Create variables to the interface directories for our IPC API.
set(INTERFACE_DIR "${LEGATO_SOURCE_DIR}/interfaces/supervisor")
set(INTERFACE_API_DIR "${INTERFACE_DIR}/api")
set(INTERFACE_C_DIR "${INTERFACE_DIR}/c")

# Add the command to generate the client interface header file.  The command only gets run when it
# is needed.
add_custom_command(
    OUTPUT ${INTERFACE_C_DIR}/le_sup_interface.h
    DEPENDS ${INTERFACE_C_DIR}/le_supervisor_defs.h ${INTERFACE_API_DIR}/le_supervisor.api
    COMMENT "Generating Supervisor API IPC header file."
    COMMAND ${IFGEN_TOOL} ${INTERFACE_API_DIR}/le_supervisor.api
                          --gen-interface
                          --output-dir ${INTERFACE_C_DIR}
                          --user-include le_supervisor_defs.h
                          --name-prefix le_sup_
                          --file-prefix le_sup_)

# Add the command to generate the source files for both the client and server and the server side
# header file.  The command only gets run when it is needed.
add_custom_command(
    OUTPUT le_sup_client.c le_sup_server.c le_sup_server.h le_sup_local.h
    DEPENDS ${INTERFACE_C_DIR}/le_sup_interface.h ${INTERFACE_API_DIR}/le_supervisor.api
    COMMENT "Generating Supervisor API IPC source code."
    COMMAND ${IFGEN_TOOL} ${INTERFACE_API_DIR}/le_supervisor.api
                          --gen-client
                          --async-server
                          --gen-server
                          --gen-server-interface
                          --gen-local
                          --user-include le_supervisor_defs.h
                          --name-prefix le_sup_
                          --file-prefix le_sup_)

# Since the generated header files go into the BINARY_DIR, need to add this to the
# include path for the compiler.  Also need to add the original source directory to
# the include path, since the userInclude.h file is still located there.
add_definitions(-I${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-I${CMAKE_CURRENT_SOURCE_DIR})

# Don't try to connect to the Log Control Daemon.
add_definitions(-DNO_LOG_CONTROL)

# Build the Supervisor.
include_directories(${LEGATO_SOURCE_DIR}/interfaces/config/c)
include_directories(${LEGATO_SOURCE_DIR}/interfaces/supervisor/c)

set_legato_component(supervisor)
add_legato_internal_executable(supervisor supervisor.c app.c proc.c resourceLimits.c sandbox.c
                               le_sup_server.c)

target_link_libraries(supervisor pthread le_cfg_api)

# Build the client side library of the Superivsor API IPC.
add_library(le_sup_api SHARED
            le_sup_client.c)

# Link in the legato library when building the client library.
target_link_libraries(le_sup_api ${LEGATO_FRAMEWORK_TARGET})
