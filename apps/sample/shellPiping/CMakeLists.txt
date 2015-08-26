#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Creates application from shellPipe.adef
mkapp(shellPipe.adef)

# This is a sample application
add_dependencies(samples shellPipe)
