#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkapp(tempTest.adef)

# This is a C test
add_dependencies(tests_c tempTest)
