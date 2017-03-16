#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from the eCallTest.adef
mkapp(eCallTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_ROOT}/components/cfgEntries
)

# This is a C test
add_dependencies(tests_c eCallTest)
