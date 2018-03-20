#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from the httpGet.adef
mkapp(httpServer.adef
)

# This is a sample application
add_dependencies(samples_c httpServer)

