#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
