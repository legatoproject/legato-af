#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application using 'print' service
mkapp(printClient.adef)

# Creates application providing 'print' service
mkapp(printServer.adef)

# This is a sample application
add_dependencies(samples_c printClient printServer)
