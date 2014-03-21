#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkexe(  appCfgInstall
            appConfigInstall.c
            -i ${PROJECT_SOURCE_DIR}/interfaces/config/c
            -i ${PROJECT_SOURCE_DIR}/interfaces/configAdmin/c
            -c ${CMAKE_CURRENT_SOURCE_DIR}/components
            appConfig
            ${EXECUTABLE_OUTPUT_PATH}/lib/lible_cfg_api.so
            ${EXECUTABLE_OUTPUT_PATH}/lib/lible_cfgAdmin_api.so
            -v
        DEPENDS
            appConfigInstall.c
            components/appConfig/inc/configInstaller.h
            components/appConfig/src/configInstaller.c
            )

mkexe(  appCfgRemove
            appConfigRemove.c
            -i ${PROJECT_SOURCE_DIR}/interfaces/config/c
            -i ${PROJECT_SOURCE_DIR}/interfaces/configAdmin/c
            -c ${CMAKE_CURRENT_SOURCE_DIR}/components
            appConfig
            ${EXECUTABLE_OUTPUT_PATH}/lib/lible_cfg_api.so
            ${EXECUTABLE_OUTPUT_PATH}/lib/lible_cfgAdmin_api.so
            -v
        DEPENDS
            appConfigRemove.c
            components/appConfig/inc/configInstaller.h
            components/appConfig/src/configInstaller.c
            )


