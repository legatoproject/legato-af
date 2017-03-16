#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
#--------------------------------------------------------------------------------------------------

# Build the on-target test apps.
mkapp(badAppSB.adef)
mkapp(badAppNSB.adef)

# This is a C test
add_dependencies(tests_c badAppSB badAppNSB)
