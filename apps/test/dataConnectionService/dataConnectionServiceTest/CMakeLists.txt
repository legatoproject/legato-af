#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkapp(dcsTest.adef)

# This is a C test
add_dependencies(tests_c dcsTest)
