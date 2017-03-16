#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkapp(atClientTest.adef
)

# This is a C test
add_dependencies(tests_c atClientTest)
