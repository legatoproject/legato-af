#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

set(APP_COMPONENT audioTest)
set(APP_TARGET audioUnitTest)

set(INTERFACES_AUDIO "${LEGATO_ROOT}/interfaces")
set(LEGATO_AUDIO "${LEGATO_ROOT}/components/audio/")
set(PA_SIMU_AUDIO "${LEGATO_ROOT}/platformAdaptor/simu/components/le_pa_audio/")
set (TEST "apps/test/audio/${APP_TARGET}")
set (LEGATO_BUILD "${LEGATO_ROOT}/build/${LEGATO_TARGET}")
set(LEGATO_LIBRARY_PATH ${LEGATO_ROOT}/build/${LEGATO_TARGET}/framework/lib/liblegato.so)

#generate API interfaces
execute_process (COMMAND ifgen --gen-interface ${INTERFACES_AUDIO}/le_audio.api
                 WORKING_DIRECTORY ${LEGATO_BUILD}/${TEST})

include_directories(    ${LEGATO_ROOT}/framework/c/inc
                        ${LEGATO_ROOT}/framework/c/src
                        ${LEGATO_AUDIO}
                        ${LEGATO_AUDIO}/platformAdaptor/inc
                        ${LEGATO_BUILD}/${TEST}
                        ${PA_SIMU_AUDIO}
                        ${LEGATO_ROOT}/${TEST}
                   )

set(LE_SOURCES
    ${LEGATO_AUDIO}/le_audio.c
    ${LEGATO_AUDIO}/le_media.c
    )

set_source_files_properties(${LEGATO_AUDIO}/le_audio.c
                            PROPERTIES COMPILE_FLAGS "-DCOMPONENT_INIT=\"void le_audio_Init()\"")

add_library(leAudioLib STATIC
            ${LE_SOURCES})


set(PA_SOURCES
    ${PA_SIMU_AUDIO}/pa_audio_simu.c
    ${PA_SIMU_AUDIO}/pa_pcm_simu.c
    ${PA_SIMU_AUDIO}/pa_amr_simu.c
)

set_source_files_properties(${PA_SIMU_AUDIO}/pa_audio_simu.c
                            PROPERTIES COMPILE_FLAGS "-DCOMPONENT_INIT=\"void pa_audio_Init()\"")


add_library(paAudioSimuLib STATIC
            ${PA_SOURCES})

set(TEST_SOURCES
    ${LEGATO_ROOT}/${TEST}/main.c
   )

add_executable ( ${APP_TARGET} ${TEST_SOURCES} )

add_definitions( "-Dle_msg_AddServiceCloseHandler=MyAddServiceCloseHandler"
                 "-Dle_msg_GetServiceRxMsg=MyGetServiceRxMsg"
                 "-Dle_msg_GetSession=MyGetSession"
                 "-Dle_msg_CloseSession=MyCloseSession" )

target_link_libraries(${APP_TARGET}
                      leAudioLib
                      paAudioSimuLib
                      ${LEGATO_LIBRARY_PATH}
                      -lpthread -lrt -lm)

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})

