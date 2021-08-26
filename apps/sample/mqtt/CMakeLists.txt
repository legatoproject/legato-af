#*******************************************************************************
# Copyright (C) Sierra Wireless, Inc.
#*******************************************************************************

mkapp(mqttPublisher.adef
  -i ${LEGATO_ROOT}/apps/platformServices/mqttClient
  -i ${LEGATO_ROOT}/apps/sample/dataHub/interfaces
  -i ${LEGATO_ROOT}/apps/sample/dataHub/interfaces/linux)
mkapp(mqttSubscriber.adef)

# This is a sample application
add_dependencies(samples_c mqttPublisher mqttSubscriber)
