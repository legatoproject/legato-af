#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from eCallDemo.adef
mkapp(eCallDemo.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_ROOT}/interfaces/positioning
    -i ${LEGATO_ROOT}/interfaces/
    -i eCallAppComponent
)

# This is a sample application
add_dependencies(samples_c eCallDemo)

generate_header(eCallAppComponent/ecallApp.api)
