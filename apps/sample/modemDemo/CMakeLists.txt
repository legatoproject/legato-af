#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

# Creates application from the .adef
mkapp(modemDemo.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
)

# This is a sample application
add_dependencies(samples modemDemo)
