#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
#*******************************************************************************

find_package(Legato REQUIRED)

# Creates application using GPIO Cf3 gpioService from gpioCf3Demo.adef
mkapp(gpioCf3Demo.adef
    -i ${LEGATO_ROOT}/interfaces/gpioService
    DEPENDS
        legato
        ## TODO: Remove this when the mk tools do dependency checking.
        gpioCf3PinComponent/gpioCf3Pin.c
        gpioCf3PinComponent/Component.cdef
)

# This is a sample application
add_dependencies(samples gpioCf3Demo)
