# Toolchain base dir
SET(WP7_TOOLCHAIN_DIR /opt/swi/tmp/sysroots/x86_64-linux/usr/bin/armv7a-vfp-neon-poky-linux-gnueabi)

# Specify the cross compiler
SET(CMAKE_C_COMPILER   ${WP7_TOOLCHAIN_DIR}/arm-poky-linux-gnueabi-gcc)
SET(CMAKE_CXX_COMPILER  ${WP7_TOOLCHAIN_DIR}/arm-poky-linux-gnueabi-gcc)

# Where is the target environment: not need for now.
#SET(CMAKE_FIND_ROOT_PATH ${WP7_TOOLCHAIN_DIR})

SET(DEFAULT_BUILD true)