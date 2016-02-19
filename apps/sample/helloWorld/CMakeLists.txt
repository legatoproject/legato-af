#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

# Creates application from helloWorld.adef
mkapp(helloWorld.adef)

# This is a sample application
add_dependencies(samples helloWorld)
