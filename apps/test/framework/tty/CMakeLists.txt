#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(APP_TARGET testTty)

mkexe(  ${APP_TARGET}
            ttyTest.c
            -i ${PROJECT_SOURCE_DIR}/framework/c/src
        )
