#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_TARGET testFwUsers)

mkexe(  ${APP_TARGET}
            userTest.c
            -i ${PROJECT_SOURCE_DIR}/framework/c/src
        DEPENDS
            userTest.c
        )
