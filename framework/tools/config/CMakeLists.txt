#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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

add_dependencies(config jansson)
