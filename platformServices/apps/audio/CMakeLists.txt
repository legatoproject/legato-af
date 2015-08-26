#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# Use of this work is subject to license.
#*******************************************************************************

message(STATUS "Audio Platform Adaptor (LEGATO_AUDIO_PA): ${LEGATO_AUDIO_PA}")

# Directory in which the audio component can be found.
set (AUDIO_COMPONENT_DIR    ${LEGATO_ROOT}/components/audio)

if (LEGATO_AUDIO_PA MATCHES ".*/pre-built/.*")

    message(STATUS "Using pre-built audio PA.")

    file(COPY ${LEGATO_AUDIO_PA}/libComponent_le_pa_audio.so DESTINATION ${LIBRARY_OUTPUT_PATH})

    set(LINK_DIRECTIVES     --ldflags=-lqmi_cci
                            --ldflags=-lqmiservices
                            --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qcsi.a
                            )

elseif (NOT DEFINED LEGATO_COMPONENTS_AUDIO)

    message(FATAL_ERROR "Legato Audio Platform Adaptor (LEGATO_COMPONENTS_AUDIO) not selected.")

elseif ((LEGATO_COMPONENTS_AUDIO MATCHES "ar7") OR
        (LEGATO_COMPONENTS_AUDIO MATCHES "ar86") OR
        (LEGATO_COMPONENTS_AUDIO MATCHES "wp7") OR
        (LEGATO_COMPONENTS_AUDIO MATCHES "wp85"))

    message(STATUS "Building audio PA from sources.")

    ## TODO: Remove this when the mk tools do dependency checking.
    set (AUDIO_PA_SOURCES
            ${LEGATO_AUDIO_PA}/pa_audio.c
            ${LEGATO_AUDIO_PA}/pa_media.c
            ${LEGATO_AUDIO_PA}/swiQmi.c
            ${LEGATO_AUDIO_PA}/swiQmi.h
        )

    set (AUDIO_PA_PROPRIETARY_INCLUDES
            -i ${LEGATO_ROOT}/proprietary/qct/inc/framework
            -i ${LEGATO_ROOT}/proprietary/qct/inc/services
        )

    set(LINK_DIRECTIVES     --ldflags=-lqmi_cci
                            --ldflags=-lqmiservices
                            --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qcsi.a
                            )

elseif (LEGATO_COMPONENTS_AUDIO MATCHES "simu")

    message(STATUS "Building simulation audio PA from sources.")

    ## TODO: Remove this when the mk tools do dependency checking.
    set (AUDIO_PA_SOURCES
            ${LEGATO_AUDIO_PA}/pa_audio_simu.c
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
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/interfaces/audio
            -i ${AUDIO_COMPONENT_DIR}
            -i ${AUDIO_COMPONENT_DIR}/platformAdaptor
            ${AUDIO_PA_PROPRIETARY_INCLUDES}
            -v
            ${LINK_DIRECTIVES}
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${AUDIO_COMPONENT_DIR}/Component.cdef
            ${AUDIO_COMPONENT_DIR}/le_audio.c
            ${AUDIO_COMPONENT_DIR}/le_media.c
            ${LEGATO_AUDIO_PA}/Component.cdef
            ${LEGATO_ROOT}/interfaces/audio/le_audio.api
            ${AUDIO_PA_SOURCES}
        )
