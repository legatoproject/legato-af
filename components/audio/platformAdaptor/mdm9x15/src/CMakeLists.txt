#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# todo: For now, only define the component name for QMI. Later there may
#       be a common name for both the QMI and AT implementations.
set_legato_component(AudioQmiPa)

if(LEGATO_COMPONENTS_AUDIO MATCHES "wp7")

    set(LEGATO_COMPONENTS_AUDIO_QMI_SRC
        ${LEGATO_COMPONENTS_AUDIO_QMI_SRC}
        pa_audio_wp7.c
    )

elseif(LEGATO_COMPONENTS_AUDIO MATCHES "ar7")

    set(LEGATO_COMPONENTS_AUDIO_QMI_SRC
        ${LEGATO_COMPONENTS_AUDIO_QMI_SRC}
        pa_audio_ar7.c
    )

else()
    message(FATAL_ERROR "Legato Audio platform to build is invalid.")
endif()

#Common
set(LEGATO_COMPONENTS_AUDIO_QMI_SRC
    ${LEGATO_COMPONENTS_AUDIO_QMI_SRC}
    pa_audio.c
)

add_library(${LEGATO_COMPONENTS_AUDIO_TARGET} SHARED
    ${LEGATO_COMPONENTS_AUDIO_QMI_SRC}
)

target_link_libraries(${LEGATO_COMPONENTS_AUDIO_TARGET}
    alsa_intf
    ${LEGATO_FRAMEWORK_TARGET}
)
