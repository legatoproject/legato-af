#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(APP_TARGET testFwUsers)

mkexe(  ${APP_TARGET}
            userTest.c
            -i ${PROJECT_SOURCE_DIR}/framework/liblegato/linux
        )

# This is a C test
add_dependencies(tests_c ${APP_TARGET})
