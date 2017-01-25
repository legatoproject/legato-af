#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
#--------------------------------------------------------------------------------------------------

# Build the on-target test apps.
mkapp(FaultApp.adef)
mkapp(RestartApp.adef)
mkapp(StopApp.adef)
mkapp(ForkChildApp.adef)
mkapp(NonSandboxedFaultApp.adef)
mkapp(NonSandboxedRestartApp.adef)
mkapp(NonSandboxedStopApp.adef)
mkapp(NonSandboxedForkChildApp.adef)
