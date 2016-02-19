#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

set(APP_TARGET testFwUsers)

mkexe(  ${APP_TARGET}
            userTest.c
            -i ${PROJECT_SOURCE_DIR}/framework/c/src
        )
