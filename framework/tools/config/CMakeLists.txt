#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)


set(JANSSON_INC_DIR "${CMAKE_BINARY_DIR}/3rdParty/jansson-2.6/include/")

mkexe(  config
            ../config
            -i ${JANSSON_INC_DIR}
            -i ../../c/src
            --ldflags=-ljansson
        DEPENDS
            config.c
        )

target_link_libraries(config le_cfg_api le_cfgAdmin_api jansson)
