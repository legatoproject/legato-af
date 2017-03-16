#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkapp(fsTest.adef
      -i ${LEGATO_ROOT}/interfaces/fsServices)

# This is a C test
add_dependencies(tests_c fsTest)
