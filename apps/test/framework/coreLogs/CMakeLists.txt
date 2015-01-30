#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#--------------------------------------------------------------------------------------------------

find_package(Legato REQUIRED)

# Build the on-target test apps.
mkapp(badAppSB.adef)
mkapp(badAppNSB.adef)

