#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
#
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

# This is a C test
add_dependencies(tests_c gnssTest)
