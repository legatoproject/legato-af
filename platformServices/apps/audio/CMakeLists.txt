#*******************************************************************************
# Copyright (C) 2013-2014, Sierra Wireless Inc.
# Use of this work is subject to license.
#*******************************************************************************

message(STATUS "Audio Platform Adaptor (LEGATO_AUDIO_PA): ${LEGATO_AUDIO_PA}")

# Directory in which the audio component can be found.
set (AUDIO_COMPONENT_DIR    ${LEGATO_ROOT}/components/audio)


if (LEGATO_AUDIO_PA MATCHES ".*/pre-built/.*")

    message(STATUS "Using pre-built audio PA.")

elseif (NOT DEFINED LEGATO_COMPONENTS_AUDIO)

    message(FATAL_ERROR "Legato Audio Platform Adaptor (LEGATO_COMPONENTS_AUDIO) not selected.")

elseif (LEGATO_COMPONENTS_AUDIO MATCHES "ar7")

    message(STATUS "Building AR7 audio PA from sources.")

    ## TODO: Remove this when the mk tools do dependency checking.
    set (AUDIO_PA_SOURCES
            ${LEGATO_AUDIO_PA}/pa_audio_ar7.c
            ${LEGATO_AUDIO_PA}/pa_audio.c
            ${LEGATO_AUDIO_PA}/pa_audio_local.h
        )

elseif (LEGATO_COMPONENTS_AUDIO MATCHES "wp7")

    message(STATUS "Building WP7 audio PA from sources.")

    ## TODO: Remove this when the mk tools do dependency checking.
    set (AUDIO_PA_SOURCES
            ${LEGATO_AUDIO_PA}/pa_audio_wp7.c
            ${LEGATO_AUDIO_PA}/pa_audio.c
            ${LEGATO_AUDIO_PA}/pa_audio_local.h
        )

elseif (LEGATO_COMPONENTS_AUDIO MATCHES "STUB")

    message(STATUS "Building stub audio PA from sources.")

    ## TODO: Remove this when the mk tools do dependency checking.
    set (AUDIO_PA_SOURCES
            ${LEGATO_AUDIO_PA}/pa_audio.c
        )

else()

    message(FATAL_ERROR "Invalid LEGATO_COMPONENTS_AUDIO Target '${LEGATO_COMPONENTS_AUDIO}'")

endif()


mkapp(  audioService.adef
            -i ${LEGATO_ROOT}/interfaces/audio
            -i ${AUDIO_COMPONENT_DIR}/platformAdaptor
            -v
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${AUDIO_COMPONENT_DIR}/Component.cdef
            ${AUDIO_COMPONENT_DIR}/le_audio.c
            ${LEGATO_AUDIO_PA}/Component.cdef
            ${LEGATO_ROOT}/interfaces/audio/le_audio.api
            ${AUDIO_PA_SOURCES}
        )
