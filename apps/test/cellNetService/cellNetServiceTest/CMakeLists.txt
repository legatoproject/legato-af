#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkapp(cellNetServiceTest.adef
      -i ${LEGATO_ROOT}/interfaces/modemServices)

# This is a C test
add_dependencies(tests_c cellNetServiceTest)
