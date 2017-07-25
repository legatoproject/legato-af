#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from the .adef
mkapp(externalWdog.adef)
mkapp(monitoredService.adef)

# This is a sample application
add_dependencies(samples_c externalWdog monitoredService)
