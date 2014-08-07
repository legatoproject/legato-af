#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Creates application from helloWorld.adef
mkapp(helloWorld.adef
    DEPENDS
        legato
        helloWorld.c
)

# This is a sample application
add_dependencies(samples helloWorld)
