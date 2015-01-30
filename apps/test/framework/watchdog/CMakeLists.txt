#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#--------------------------------------------------------------------------------------------------

find_package(Legato REQUIRED)

# Build the on-target test apps.
mkapp(dogTest.adef)
mkapp(dogTestNever.adef)
mkapp(dogTestNeverNow.adef)
mkapp(dogTestRevertAfterTimeout.adef)
mkapp(dogTestWolfPack.adef)
