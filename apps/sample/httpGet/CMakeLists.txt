#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from the httpGet.adef
mkapp(httpGet.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
)

# This is a sample application
add_dependencies(samples_c httpGet)

