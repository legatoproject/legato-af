#*******************************************************************************
# Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

mkapp(lwm2mControl.adef
    -i ${LEGATO_SOURCE_DIR}/interfaces/airVantage/
)

# This is a sample application
add_dependencies(samples lwm2mControl)