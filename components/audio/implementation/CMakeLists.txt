#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set_legato_component(Audio)

include_directories(
    ${LEGATO_INCLUDE_DIRS}
    ${LEGATO_INCLUDE_DIRS_PRIV}
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src LEGATO_SERVICES_AUDIO_SRC)

add_library(${LEGATO_SERVICES_AUDIO_TARGET} SHARED
    ${LEGATO_SERVICES_AUDIO_SRC}
)

target_link_libraries(${LEGATO_SERVICES_AUDIO_TARGET}
    ${LEGATO_COMPONENTS_AUDIO_TARGET}
    ${LEGATO_FRAMEWORK_TARGET}
)

