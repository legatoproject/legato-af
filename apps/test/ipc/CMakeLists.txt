#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

find_package(CUnit REQUIRED)

mkapp(ipcTestC2C.adef
  -i interfaces
  --cflags=-I${CUNIT_INSTALL}/include
  --ldflags="${CUNIT_LIBRARIES}")

mkapp(ipcTestC2Java.adef
  -i interfaces
  -s ${LEGATO_ROOT}/components
  --cflags=-I${CUNIT_INSTALL}/include
  --ldflags="${CUNIT_LIBRARIES}")
