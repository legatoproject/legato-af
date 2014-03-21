#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved. 
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

# Creates application from the .adef
mkapp(airvantageDemoApp airvantageDemo.adef
    -i ${AIRVANTAGE_INCLUDE_DIR}
    DEPENDS
        legato
        airvantageDemo.c
)

# This is a sample application
add_dependencies(samples airvantageDemoApp)