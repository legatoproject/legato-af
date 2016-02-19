#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

# Creates application from eCallDemo.adef
mkapp(eCallDemo.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_ROOT}/interfaces/positioning
    -i ${LEGATO_ROOT}/interfaces/
    -i eCallAppComponent
)

# This is a sample application
add_dependencies(samples eCallDemo)

generate_header(eCallAppComponent/ecallApp.api)
