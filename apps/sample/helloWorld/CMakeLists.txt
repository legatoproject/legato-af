#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from helloWorld.adef
mkapp(helloWorld.adef)

# This is a sample application
add_dependencies(samples_c helloWorld)
