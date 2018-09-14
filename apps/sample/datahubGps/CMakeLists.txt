#*******************************************************************************
# Copyright (C) Sierra Wireless, Inc.
#*******************************************************************************

mkapp(gpsSensor.adef)
mkapp(gpsMonitor.adef)

# This is a sample application
add_dependencies(samples_c gpsSensor gpsMonitor)
