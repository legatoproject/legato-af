#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc., all rights reserved.
#
#*******************************************************************************

find_package(Legato REQUIRED)

# Creates application from eCall.adef
mkapp(eCall.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_ROOT}/interfaces/positioning
    -i ${LEGATO_ROOT}/interfaces/
    -i eCallAppComponent
    DEPENDS
        legato
        ## TODO: Remove this when the mk tools do dependency checking.
        eCallAppComponent/Component.cdef
        eCallAppComponent/ecallApp.api
        eCallAppComponent/eCallApp.c
        triggerComponent/Component.cdef
        triggerComponent/trigger.c
)

# This is a sample application
add_dependencies(samples eCall)
