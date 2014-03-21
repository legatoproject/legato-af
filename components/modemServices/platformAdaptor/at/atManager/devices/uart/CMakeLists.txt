#*******************************************************************************
# Copyright (c) 2013 Sierra Wireless and others.
# All rights reserved. Use of this work is subject to license. 
#*******************************************************************************

project(LEGATO_DEVICE_UART C)

find_package(Legato REQUIRED)

###
### UART
###

# Includes
include_directories(
    ${LEGATO_INCLUDE_DIRS}
    ${LEGATO_INCLUDE_DIRS_PRIV}
)

# Sources
aux_source_directory(src LEGATO_UART_SRC)

# Builds the legato library
add_library(${LEGATO_UART_TARGET} SHARED ${LEGATO_UART_SRC})

# Linking
target_link_libraries(${LEGATO_FRAMEWORK_TARGET})
