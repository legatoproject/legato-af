#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************


mkexe(  inspect
            inspect.c
            -i ${PROJECT_SOURCE_DIR}/framework/c/src
        DEPENDS
            inspect.c
            ${PROJECT_SOURCE_DIR}/framework/c/src/mem.h
            ${PROJECT_SOURCE_DIR}/framework/c/src/mem.c
            )
