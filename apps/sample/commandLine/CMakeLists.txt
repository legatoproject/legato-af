#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
#--------------------------------------------------------------------------------------------------

# Creates application from helloWorld.adef
mkapp(fileInfo.adef)

# This is a sample application
add_dependencies(samples_c fileInfo)
