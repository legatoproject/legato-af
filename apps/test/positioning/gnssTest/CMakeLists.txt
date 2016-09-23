#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Use of this work is subject to license.
#*******************************************************************************

# Creates application from the gnssTest.adef

if(LEGATO_TARGET STREQUAL "ar759x")

mkapp(gnssTest.adef
    -i ${LEGATO_ROOT}/interfaces/positioning
    -C "-DSIERRA_MDM9X40"
)

elseif(LEGATO_TARGET STREQUAL "ar758x")

mkapp(gnssTest.adef
    -i ${LEGATO_ROOT}/interfaces/positioning
    -C "-DSIERRA_MDM9X28"
)

else()

mkapp(gnssTest.adef
    -i ${LEGATO_ROOT}/interfaces/positioning
)

endif()
