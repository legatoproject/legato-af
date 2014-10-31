#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_TARGET testPaAudio)

if(${LEGATO_TARGET} STREQUAL "ar7")

    include_directories(
        ${CUNIT_INCLUDE_DIRS}
    )

    message("CUNIT_INCLUDE_DIRS: ${CUNIT_INCLUDE_DIRS}")

    mkexe(${APP_TARGET}
                .
                ${CUNIT_LIBRARIES}
                -i ${CUNIT_INSTALL}/include
                -i ${CUNIT_INSTALL}/include/CUnit
                -i ${LEGATO_ROOT}/interfaces/audio
                -i ${LEGATO_ROOT}/components/audio/platformAdaptor
                -i ${LEGATO_ROOT}/proprietary/qct/inc/framework
                -i ${LEGATO_ROOT}/proprietary/qct/inc/services
                --ldflags=-lqmi_cci
                --ldflags=-lqmiservices
                --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qcsi.a
            DEPENDS
                legato
                ## TODO: Remove this when the mk tools do dependency checking.
                Component.cdef
                testPaAudioAr7.c
            )


    if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
        add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
        set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
    endif()

endif()
