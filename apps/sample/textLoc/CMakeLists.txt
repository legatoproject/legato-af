#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

# Creates application from textLoc.adef
mkapp(textLoc.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_ROOT}/interfaces/positioning
    -i ${LEGATO_ROOT}/interfaces
)

# This is a sample application
add_dependencies(samples textLoc)
