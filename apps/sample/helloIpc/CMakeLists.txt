#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

# Creates application using 'print' service
mkapp(printClient.adef)

# Creates application providing 'print' service
mkapp(printServer.adef)

# This is a sample application
add_dependencies(samples printClient printServer)
