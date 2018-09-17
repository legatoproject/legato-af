#*******************************************************************************
# Copyright (C) Sierra Wireless, Inc.
#*******************************************************************************

mkapp(mqttPublisher.adef)
mkapp(mqttSubscriber.adef)

# This is a sample application
add_dependencies(samples_c mqttPublisher mqttSubscriber)
