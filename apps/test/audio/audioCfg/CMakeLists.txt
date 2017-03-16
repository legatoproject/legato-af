#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkapp(audioCfgTest.adef)

# This is a C test
add_dependencies(tests_c audioCfgTest)
