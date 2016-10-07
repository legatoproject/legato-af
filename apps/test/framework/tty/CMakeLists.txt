#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

set(APP_TARGET testTty)

mkexe(  ${APP_TARGET}
            ttyTest.c
            -i ${PROJECT_SOURCE_DIR}/framework/c/src
        )
