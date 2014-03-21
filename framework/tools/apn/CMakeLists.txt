#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkexe(  apn
            apn.c
            -i ${PROJECT_SOURCE_DIR}/interfaces/config/c
            -c ${CMAKE_CURRENT_SOURCE_DIR}/components
            ${EXECUTABLE_OUTPUT_PATH}/lib/lible_cfg_api.so
            -v
            )