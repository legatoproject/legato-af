#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkapp(atServerApp.adef)

# This is a C test
add_dependencies(tests_c atServerApp)
