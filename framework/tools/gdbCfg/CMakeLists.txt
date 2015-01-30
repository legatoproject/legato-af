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
