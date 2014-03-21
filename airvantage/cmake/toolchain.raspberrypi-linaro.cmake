# this one is important
#SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
#SET(CMAKE_SYSTEM_VERSION 1)

#########
# linaro toolchain can be downloaded from:
# https://github.com/raspberrypi/tools.git
# get it
# then configure next var using the place where the toolchain was outputed)
GET(ENV{HOME} HOME)
SET(RASPI_LINARO_BASE ${HOME}/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/)

# specify the cross compiler
SET(CMAKE_C_COMPILER   ${RASPI_LINARO_BASE}/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER  ${RASPI_LINARO_BASE}/bin/arm-linux-gnueabihf-g++)

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH ${RASPI_LINARO_BASE})

# search for programs in the build host directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(DEFAULT_BUILD true)

SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
