#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

# Creates application using GPIO Cf3 gpioService from gpioCf3Demo.adef
mkapp(gpioCf3Demo.adef
    -i ${LEGATO_ROOT}/interfaces/gpioService
)

# This is a sample application
add_dependencies(samples gpioCf3Demo)
