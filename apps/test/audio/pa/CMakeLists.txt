#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_COMPONENT AUDIOTest)
set(APP_TARGET testAudio)

if(${LEGATO_TARGET} STREQUAL "ar7")

    set(APP_SOURCES
        test_pa_audio_ar7.c
    )

    include_directories(
        ${CUNIT_INCLUDE_DIRS}
        ${LEGATO_COMPONENTS_AUDIO_DIR}/qmi/src
    )

    set_legato_component(${APP_COMPONENT})
    add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

    target_link_libraries(${APP_TARGET}
        ${CUNIT_LIBRARIES}
        ${LEGATO_COMPONENTS_AUDIO_TARGET}
    )

    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()

