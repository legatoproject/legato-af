#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from the powerMgrTest.adef
mkapp(powerMgrTest.adef)

# This is a C test
add_dependencies(tests_c powerMgrTest)
