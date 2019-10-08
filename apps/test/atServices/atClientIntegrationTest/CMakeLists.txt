#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from atClientIntegrationTest.adef
mkapp(atClientIntegrationTest.adef)

# This is a sample application
add_dependencies(samples_c atClientIntegrationTest)
