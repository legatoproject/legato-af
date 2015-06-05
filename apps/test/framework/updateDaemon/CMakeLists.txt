#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#--------------------------------------------------------------------------------------------------

find_package(Legato REQUIRED)

# Build the on-target test apps.
mkapp(updateFaultApp.adef)
mkapp(updateRestartApp.adef)
mkapp(updateStopApp.adef)
mkapp(updateNonSandboxedFaultApp.adef)
mkapp(updateNonSandboxedRestartApp.adef)
mkapp(updateNonSandboxedStopApp.adef)
