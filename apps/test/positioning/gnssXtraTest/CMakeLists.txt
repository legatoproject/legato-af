#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Use of this work is subject to license.
#*******************************************************************************

# Creates application from the gnssXtraTest.adef
mkapp(gnssXtraTest.adef
    -i ${LEGATO_ROOT}/interfaces/positioning
    -i ${LEGATO_ROOT}/interfaces/modemServices
)
