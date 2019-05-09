#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
#--------------------------------------------------------------------------------------------------

# Build host unit tests.

set(LEGATO_BUILD            ${LEGATO_ROOT}/build/${LEGATO_TARGET})
set(LEGATO_BIN_DIR          ${LEGATO_BUILD}/framework/bin)

set(SERVICE_DIRECTORY_BIN   ${LEGATO_BIN_DIR}/serviceDirectory)
set(LOG_CTRL_DAEMON_BIN     ${LEGATO_BIN_DIR}/logCtrlDaemon)
set(CONFIG_TREE_BIN         ${LEGATO_BIN_DIR}/configTree)
set(CONFIG_TOOL_BIN         ${LEGATO_BIN_DIR}/config)


configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/configTest.sh.in
    ${EXECUTABLE_OUTPUT_PATH}/configTest.sh
    @ONLY
)


mkexe(configDropReadExe
      configDropRead)

mkexe(configDropWriteExe
      configDropWrite)


mkexe(configTestExe
      configTest)


mkexe(configDelete
      configDelete)

# This is a C test
add_dependencies(tests_c configDropReadExe
                         configDropWriteExe
                         configTestExe
                         configDelete)

add_test(configTest ${EXECUTABLE_OUTPUT_PATH}/configTest.sh)


# On-target test apps.

mkapp(cfgSelfRead.adef)
mkapp(cfgSelfWrite.adef)
mkapp(cfgSystemRead.adef)
mkapp(cfgSystemWrite.adef)

# This is a C test
add_dependencies(tests_c cfgSelfRead cfgSelfWrite cfgSystemRead cfgSystemWrite)
