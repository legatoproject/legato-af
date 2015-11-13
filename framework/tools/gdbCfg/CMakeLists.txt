#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkexe(  gdbCfg
            ../gdbCfg
            -i ${PROJECT_SOURCE_DIR}/framework/c/src
            -i ${PROJECT_SOURCE_DIR}/interfaces/config/c
            -i ${PROJECT_SOURCE_DIR}/interfaces/configAdmin/c
            -c ../installer/components
            appConfig
            -i ../installer/components/appConfig/inc
            -v
        DEPENDS
            gdbCfg.c
            ../installer/components/appConfig/inc/configInstaller.h
            ../installer/components/appConfig/src/configInstaller.c
            )

add_custom_command(OUTPUT ${EXECUTABLE_OUTPUT_PATH}/straceCfg
                   COMMAND ln -sf gdbCfg straceCfg
                   WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
                   DEPENDS gdbCfg
                   )

add_custom_target(straceCfg
                  ALL
                  DEPENDS ${EXECUTABLE_OUTPUT_PATH}/straceCfg
                  )

add_dependencies(straceCfg ${LEGATO_FRAMEWORK_TARGET})

