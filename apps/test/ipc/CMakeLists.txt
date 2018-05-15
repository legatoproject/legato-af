#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

find_package(CUnit REQUIRED)

mkapp(ipcTestC2C.adef
  -i interfaces
  --cflags=-I${CUNIT_INSTALL}/include
  --ldflags="${CUNIT_LIBRARIES}")

# This is a C test
add_dependencies(ipcTestC2C cunit)
add_dependencies(tests_c ipcTestC2C)

mkapp(ipcTestC2Java.adef
  -i interfaces
  -s ${LEGATO_ROOT}/components
  --cflags=-I${CUNIT_INSTALL}/include
  --ldflags="${CUNIT_LIBRARIES}")

# This is a Java test
add_dependencies(ipcTestC2Java cunit)
add_dependencies(tests_java ipcTestC2Java)
