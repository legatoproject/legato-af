#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application using GPIO Cf3 gpioService from gpioCf3DemoSS.adef
mkapp(gpioCf3SSDemo.adef
    -i ${LEGATO_ROOT}/interfaces/gpioService
)

# This is a sample application
add_dependencies(samples_c gpioCf3SSDemo)
