#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(TEST_NAME testIfGen2)
set_legato_component(${TEST_NAME})

# Set the path to the ifgen tool
set(IFGEN_TOOL ${LEGATO_ROOT}/bin/ifgen )

# todo: There may be a better way to generate the imported file(s), as this is
#       somewhat awkward to use.
add_custom_command (
    OUTPUT common_interface.h common_server.h common_common.h
    COMMAND ${IFGEN_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/common.api
                          --gen-interface
                          --gen-common-interface
                          --gen-server-interface
    DEPENDS common.api
)


add_custom_command (
    OUTPUT example_client.c example_server.c example_common.h example_commonclient.c
    COMMAND ${IFGEN_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/example.api
                          --gen-all --name-prefix=example
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

add_legato_internal_executable(${TEST_CLIENT}
  example_client.c example_commonclient.c
  clientMain.c)
add_legato_internal_executable(${TEST_SERVER}
  example_server.c
  serverMain.c)

# This is a C test
add_dependencies(tests_c ${TEST_CLIENT} ${TEST_SERVER})

# This goes into the "tests" directory, with all the other executables
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT}.in
               ${EXECUTABLE_OUTPUT_PATH}/${TEST_SCRIPT})

#
# Build multi client test
#
set(TEST_SCRIPT testMulti2.sh)
set(TEST_CLIENT testMulti2_client)
set(TEST_SERVER testMulti2_server)

add_legato_internal_executable(${TEST_CLIENT}
  example_client.c example_commonclient.c
  multiClientMain.c)
add_legato_internal_executable(${TEST_SERVER}
  example_server.c
  multiServerMain.c)

# This is a C test
add_dependencies(tests_c ${TEST_CLIENT} ${TEST_SERVER})

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
                          --name-prefix=async
    DEPENDS example.api common_interface.h example_common.h
)


set(TEST_SCRIPT testAsync2.sh)
set(TEST_CLIENT testAsync2_client)
set(TEST_SERVER testAsync2_server)

add_legato_internal_executable(${TEST_CLIENT}
  example_client.c example_commonclient.c clientMain.c)
add_legato_internal_executable(${TEST_SERVER}
  async_server.c asyncServerMain.c)

# This is a C test
add_dependencies(tests_c ${TEST_CLIENT} ${TEST_SERVER})

# This goes into the "tests" directory, with all the other executables
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT}.in
               ${EXECUTABLE_OUTPUT_PATH}/${TEST_SCRIPT})


#
# Build .api sharing test
#
set(TEST_SCRIPT testApiSharing.sh)
set(TEST_CLIENT testApiSharing_client)
set(TEST_SERVER testApiSharing_server)

mkexe(${TEST_CLIENT} ${TEST_CLIENT})
mkexe(${TEST_SERVER} ${TEST_SERVER})

# This is a C test
add_dependencies(tests_c ${TEST_CLIENT} ${TEST_SERVER})

# This goes into the "tests" directory, with all the other executables
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT}.in
               ${EXECUTABLE_OUTPUT_PATH}/${TEST_SCRIPT})

#
# Build client using empty.api
#

set(TEST_CLIENT testEmptyApi_client)
mkexe(${TEST_CLIENT} ${TEST_CLIENT})

# This is a C test
add_dependencies(tests_c ${TEST_CLIENT})

