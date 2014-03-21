#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# this one is important
#SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
#SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
#SET(CMAKE_C_COMPILER   /opt/kerlink/bin/arm-linux-gcc)
#SET(CMAKE_CXX_COMPILER /opt/kerlink/bin/arm-linux-g++)

# where is the target environment
#SET(CMAKE_FIND_ROOT_PATH  /opt/kerlink/)

# search for programs in the build host directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(DEFAULT_BUILD true)

find_program(FIND_DPKG "dpkg")

if(${FIND_DPKG} STREQUAL "FIND_DPKG-NOTFOUND")
    execute_process(COMMAND uname -m OUTPUT_VARIABLE UNAME_ARCH_TMP)
    string(REPLACE "\n" "" UNAME_ARCH ${UNAME_ARCH_TMP})
    if (${UNAME_ARCH} STREQUAL "i686")
      SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386") # TODO CHANGEME to amd64, or whatever you want
    elseif(${UNAME_ARCH} STREQUAL "x86_64")
      SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    endif()
else(${FIND_DPKG} STREQUAL "FIND_DPKG-NOTFOUND")
    execute_process(COMMAND dpkg --print-architecture OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE_TMP)
    string(REPLACE "\n" "" CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE_TMP})
endif(${FIND_DPKG} STREQUAL "FIND_DPKG-NOTFOUND")

find_program(FIND_RPM "rpmbuild")

if(${FIND_RPM} STREQUAL "FIND_RPM-NOTFOUND")
    SET(CPACK_GENERATOR "TGZ;DEB")
else(${FIND_RPM} STREQUAL "FIND_RPM-NOTFOUND")
    SET(CPACK_GENERATOR "TGZ;DEB;RPM")
endif(${FIND_RPM} STREQUAL "FIND_RPM-NOTFOUND")
