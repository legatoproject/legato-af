#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#--------------------------------------------------------------------------------------------------

find_package(Legato REQUIRED)

# Build the on-target test apps.
mkapp(FaultApp.adef)
mkapp(RestartApp.adef)
mkapp(StopApp.adef)
mkapp(NonSandboxedFaultApp.adef)
mkapp(NonSandboxedRestartApp.adef)
mkapp(NonSandboxedStopApp.adef)
