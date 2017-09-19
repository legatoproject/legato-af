#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from streamMedia.adef
mkapp(streamMedia.adef)
mkapp(streamMediaTest.adef)

# This is a sample application
add_dependencies(samples_c streamMedia)
add_dependencies(samples_c streamMediaTest)
