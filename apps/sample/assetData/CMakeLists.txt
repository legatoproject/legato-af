#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from .adef
mkapp(assetData.adef)

# This is a sample application
add_dependencies(samples_c assetData)
