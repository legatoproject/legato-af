#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

project(LEGATO_AUDIO C)

find_package(Legato REQUIRED)

###
### Audio
###

message(STATUS "Audio Component (LEGATO_COMPONENTS_AUDIO): ${LEGATO_COMPONENTS_AUDIO}")

include_directories(
    ${LEGATO_INCLUDE_DIRS}
    ${LEGATO_INCLUDE_DIRS_PRIV}
)

# Modem Platform Adapter

if(NOT DEFINED LEGATO_COMPONENTS_AUDIO)
    message(FATAL_ERROR "Legato Audio platform to build is not selected.")

# AR7 Component Implementation
elseif(LEGATO_COMPONENTS_AUDIO MATCHES "ar7")

    add_subdirectory(${LEGATO_COMPONENTS_AUDIO_DIR}/mdm9x15/src)

# WP7 Implementation
elseif(LEGATO_COMPONENTS_AUDIO MATCHES "wp7")

    add_subdirectory(${LEGATO_COMPONENTS_AUDIO_DIR}/mdm9x15/src)

# STUB Component Implementation
elseif(LEGATO_COMPONENTS_AUDIO MATCHES "STUB")

    add_subdirectory(${LEGATO_COMPONENTS_AUDIO_DIR}/stub/src)

else()
    message(FATAL_ERROR "Legato Audio platform to build is invalid.")
endif()
