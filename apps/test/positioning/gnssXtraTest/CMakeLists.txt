#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
#
#*******************************************************************************

# Creates application from the gnssXtraTest.adef
mkapp(gnssXtraTest.adef
    -i ${LEGATO_ROOT}/interfaces/positioning
    -i ${LEGATO_ROOT}/interfaces/modemServices
)

# This is a C test
add_dependencies(tests_c gnssXtraTest)
