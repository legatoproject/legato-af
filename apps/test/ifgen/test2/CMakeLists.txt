#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(TEST_NAME testIfGen2)
set_legato_component(${TEST_NAME})

# Set the path to the ifgen tool
set(IFGEN_TOOL ${LEGATO_SOURCE_DIR}/bin/ifgen )

# todo: There may be a better way to generate the imported file(s), as this is
#       somewhat awkward to use.  
add_custom_command (
    OUTPUT common_interface.h common_server.h
    COMMAND ${IFGEN_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/common.api
                          --gen-interface
                          --gen-server-interface
    DEPENDS common.api
)


add_custom_command (
    OUTPUT client.c server.c
    COMMAND ${IFGEN_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/example.api
                          --gen-all --no-default-prefix
    DEPENDS example.api common_interface.h common_server.h
)

# Since the generated header files go into the BINARY_DIR, need to add this to the
# include path for the compiler.  
add_definitions(-I${CMAKE_CURRENT_BINARY_DIR})

#
# Build single client test
#
set(TEST_SCRIPT testIfGen2.sh)
set(TEST_CLIENT testIfGen2_client)
set(TEST_SERVER testIfGen2_server)

add_legato_internal_executable(${TEST_CLIENT} client.c clientMain.c)
add_legato_internal_executable(${TEST_SERVER} server.c serverMain.c)

# This goes into the "tests" directory, with all the other executables
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT}.in
               ${EXECUTABLE_OUTPUT_PATH}/${TEST_SCRIPT})

#
# Build multi client test
#
set(TEST_SCRIPT testMulti2.sh)
set(TEST_CLIENT testMulti2_client)
set(TEST_SERVER testMulti2_server)

add_legato_internal_executable(${TEST_CLIENT} client.c multiClientMain.c)
add_legato_internal_executable(${TEST_SERVER} server.c multiServerMain.c)

# This goes into the "tests" directory, with all the other executables
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT}.in
               ${EXECUTABLE_OUTPUT_PATH}/${TEST_SCRIPT})


#
# Build server-side async test
#

add_custom_command (
    OUTPUT async_server.c async_local.h
    COMMAND ${IFGEN_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/example.api
                          --gen-server 
                          --gen-server-interface
                          --gen-local
                          --async-server
                          --file-prefix async_
                          --no-default-prefix
    DEPENDS example.api common_interface.h
)


set(TEST_SCRIPT testAsync2.sh)
set(TEST_CLIENT testAsync2_client)
set(TEST_SERVER testAsync2_server)

add_legato_internal_executable(${TEST_CLIENT} client.c clientMain.c)
add_legato_internal_executable(${TEST_SERVER} async_server.c asyncServerMain.c)

# This goes into the "tests" directory, with all the other executables
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT}.in
               ${EXECUTABLE_OUTPUT_PATH}/${TEST_SCRIPT})

