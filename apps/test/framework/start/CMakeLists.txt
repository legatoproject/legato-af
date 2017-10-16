#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from rebootTest.adef
mkapp(rebootTest.adef
    -i ${LEGATO_ROOT}/interfaces
)

# This is a test application
add_dependencies(tests_c rebootTest)
