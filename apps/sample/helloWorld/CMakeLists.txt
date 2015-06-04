#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Creates application from helloWorld.adef
mkapp(helloWorld.adef
    DEPENDS
        legato
        helloComponent/*
        *
)

# This is a sample application
add_dependencies(samples helloWorld)
