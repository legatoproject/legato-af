#*******************************************************************************
# Copyright (C) Sierra Wireless, Inc.
#*******************************************************************************

mkapp(gpsSensor.adef
  -i ${LEGATO_ROOT}/apps/sample/dataHub/interfaces
  -i ${LEGATO_ROOT}/apps/sample/dataHub/interfaces/linux)
mkapp(gpsMonitor.adef
  -i ${LEGATO_ROOT}/apps/sample/dataHub/interfaces
  -i ${LEGATO_ROOT}/apps/sample/dataHub/interfaces/linux)


# This is a sample application
add_dependencies(samples_c gpsSensor gpsMonitor)
