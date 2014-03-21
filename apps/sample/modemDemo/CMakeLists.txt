#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

# Creates application from the .adef
mkapp(modemDemoApp modemDemo.adef
    -i ${LEGATO_SOURCE_DIR}/interfaces/modemServices/c
    -i ${LEGATO_SOURCE_DIR}/interfaces/dataConnectionService/c
    DEPENDS
        legato
        le_mdm_client
        le_data_client
        send_util.api
        demoComponent/*
        sendComponent/*
)

# This is a sample application
add_dependencies(samples modemDemoApp)
