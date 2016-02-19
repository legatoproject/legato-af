#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

set(APP_COMPONENT mdcTest)
set(APP_TARGET mdcUnitTest)

set(JANSSON_INC_DIR "${CMAKE_BINARY_DIR}/framework/libjansson/include/")
set(INTERFACES_MODEM_SERVICES "${LEGATO_ROOT}/interfaces/modemServices/")
set(LEGATO_MODEM_SERVICES "${LEGATO_ROOT}/components/modemServices/")
set(PA_SIMU_MODEM_SERVICES "${LEGATO_MODEM_SERVICES}/platformAdaptor/simu/le_pa/")
set (TEST "apps/test/modemServices/mdc/${APP_TARGET}")
set (LEGATO_BUILD "${LEGATO_ROOT}/build/${LEGATO_TARGET}")
set(APNFILE "${LEGATO_ROOT}/components/modemServices/modemDaemon/apns-full-conf.json")

#generate API interfaces
execute_process (COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_mdc.api
                 COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_mdmDefs.api
                 COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_mrc.api
                 COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_sim.api
                 WORKING_DIRECTORY ${LEGATO_BUILD}/${TEST})

include_directories(    ${LEGATO_ROOT}/framework/c/inc
                        ${LEGATO_ROOT}/framework/c/src
                        ${LEGATO_MODEM_SERVICES}/modemDaemon
                        ${LEGATO_MODEM_SERVICES}/platformAdaptor/inc
                        ${PA_SIMU_MODEM_SERVICES}
                        ${LEGATO_BUILD}/${TEST}
                        ${LEGATO_ROOT}/${TEST}
                        ${LEGATO_ROOT}/components/cfgEntries
                        ${JANSSON_INC_DIR}  )

set(APP_SOURCES
    ${LEGATO_ROOT}/${TEST}/main.c
    ${LEGATO_MODEM_SERVICES}/modemDaemon/le_mdc.c
    ${LEGATO_MODEM_SERVICES}/modemDaemon/le_mrc.c
    ${LEGATO_MODEM_SERVICES}/modemDaemon/le_sim.c
    ${PA_SIMU_MODEM_SERVICES}/pa_mrc_simu.c
    ${PA_SIMU_MODEM_SERVICES}/pa_mdc_simu.c
    ${PA_SIMU_MODEM_SERVICES}/pa_sim_simu.c
)

set_legato_component(${APP_COMPONENT})

add_legato_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET} -ljansson)

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET} ${APNFILE} )
