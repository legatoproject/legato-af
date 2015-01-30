#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
