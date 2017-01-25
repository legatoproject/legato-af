#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
#--------------------------------------------------------------------------------------------------

# Build the on-target test apps.
mkapp(updateFaultApp.adef)
mkapp(updateRestartApp.adef)
mkapp(updateStopApp.adef)
mkapp(updateNonSandboxedFaultApp.adef)
mkapp(updateNonSandboxedRestartApp.adef)
mkapp(updateNonSandboxedStopApp.adef)
