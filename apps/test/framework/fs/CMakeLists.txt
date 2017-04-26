#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(APP_TARGET testFwFs)

mkexe(${APP_TARGET} main.c)

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})

# This is a C test
add_dependencies(tests_c ${APP_TARGET})
