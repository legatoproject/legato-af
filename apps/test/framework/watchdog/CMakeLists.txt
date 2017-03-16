#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
#--------------------------------------------------------------------------------------------------

# Build the on-target test apps.
mkapp(dogTest.adef)
mkapp(dogTestNever.adef)
mkapp(dogTestNeverNow.adef)
mkapp(dogTestRevertAfterTimeout.adef)
mkapp(dogTestWolfPack.adef)

mkapp(dogTestNonSandboxed.adef)

# This is a C test
add_dependencies(tests_c
                 dogTest dogTestNever dogTestNeverNow dogTestRevertAfterTimeout dogTestWolfPack
                 dogTestNonSandboxed
                 )
