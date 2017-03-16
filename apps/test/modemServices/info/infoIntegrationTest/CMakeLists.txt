#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkapp(infoTest.adef
     -i ${LEGATO_ROOT}/interfaces/modemServices
)

# This is a C test
add_dependencies(tests_c infoTest)
