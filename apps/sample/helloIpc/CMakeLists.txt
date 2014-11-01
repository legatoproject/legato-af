#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Creates application using 'print' service
mkapp(printClient.adef
    DEPENDS
        legato
        printClient/*
)

# Creates application providing 'print' service
mkapp(printServer.adef
    DEPENDS
        legato
        printServer/*
)

# This is a sample application
add_dependencies(samples printClient printServer)
