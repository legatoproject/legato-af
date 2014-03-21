
get_property(SERVICE_DIRECTORY_BIN TARGET serviceDirectory PROPERTY LOCATION)
get_property(LOG_CTRL_DAEMON_BIN TARGET logCtrlDaemon PROPERTY LOCATION)
get_property(CONFIG_TREE_BIN TARGET configTree PROPERTY LOCATION)
get_property(CONFIG_TOOL_BIN TARGET config PROPERTY LOCATION)



configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/configTest.sh.in
    ${EXECUTABLE_OUTPUT_PATH}/configTest.sh
    @ONLY
)



mkexe(configDropReadExe
      ${CMAKE_CURRENT_SOURCE_DIR}/configDropRead.c
      ${LIBRARY_OUTPUT_PATH}/lible_cfg_api.so
      -i ${LEGATO_SOURCE_DIR}/interfaces/config/c
      DEPENDS legato
              le_cfg_api
              configDropRead.c)

mkexe(configDropWriteExe
      ${CMAKE_CURRENT_SOURCE_DIR}/configDropWrite.c
      ${LIBRARY_OUTPUT_PATH}/lible_cfg_api.so
      -i ${LEGATO_SOURCE_DIR}/interfaces/config/c
      DEPENDS legato
              le_cfg_api
              configDropWrite.c)


mkexe(configTestExe
      ${CMAKE_CURRENT_SOURCE_DIR}/configTest.c
      ${LIBRARY_OUTPUT_PATH}/lible_cfg_api.so
      -i ${LEGATO_SOURCE_DIR}/interfaces/config/c
      DEPENDS legato
              le_cfg_api
              configTest.c)

add_test(configTest ${EXECUTABLE_OUTPUT_PATH}/configTest.sh)
