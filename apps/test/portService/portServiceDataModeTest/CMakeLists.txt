#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkapp(portServiceDataModeTest.adef
    -i ${LEGATO_ROOT}/interfaces/atServices
)

# This is a C test
add_dependencies(tests_c portServiceDataModeTest)
