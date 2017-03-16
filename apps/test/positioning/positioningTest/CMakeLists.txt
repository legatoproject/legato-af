#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from the positioningTest.adef
mkapp(positioningTest.adef
    -i ${LEGATO_ROOT}/interfaces/positioning
)

# This is a C test
add_dependencies(tests_c positioningTest)
