#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set_legato_component(AudioStubPa)

aux_source_directory(${LEGATO_COMPONENTS_AUDIO_DIR}/stub/src
                     LEGATO_COMPONENTS_AUDIO_STUB_SRC)

add_library(${LEGATO_COMPONENTS_AUDIO_TARGET} SHARED
            ${LEGATO_COMPONENTS_AUDIO_STUB_SRC}
)

target_link_libraries(${LEGATO_COMPONENTS_AUDIO_TARGET}
    ${LEGATO_FRAMEWORK_TARGET}
)
