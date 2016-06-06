#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

set(APP_COMPONENT ecallTest)
set(APP_TARGET ecallUnitTest)

set(INTERFACES "${LEGATO_ROOT}/interfaces")
set(INTERFACES_MODEM_SERVICES "${LEGATO_ROOT}/interfaces/modemServices/")
set(LEGATO_MODEM_SERVICES "${LEGATO_ROOT}/components/modemServices/")
set(PA_SIMU_MODEM_SERVICES "${PA_DIR}/simu/components/le_pa/")
set(SIMU_CONFIG_TREE "${CMAKE_CURRENT_SOURCE_DIR}/simu/")
set (TEST "apps/test/modemServices/ecall/${APP_TARGET}")
set (LEGATO_BUILD "${LEGATO_ROOT}/build/${LEGATO_TARGET}")

#generate API interfaces
execute_process (COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_ecall.api
                 COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_mcc.api
                 COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_mdmDefs.api
                 COMMAND ifgen --gen-interface ${INTERFACES}/le_cfg.api
                 COMMAND ifgen --gen-interface ${INTERFACES}/le_cfgAdmin.api
                 COMMAND ifgen --gen-interface ${INTERFACES}/le_pm.api
                 WORKING_DIRECTORY ${LEGATO_BUILD}/${TEST})

include_directories(    ${LEGATO_ROOT}/framework/c/inc
                        ${LEGATO_ROOT}/framework/c/src
                        ${LEGATO_MODEM_SERVICES}/modemDaemon
                        ${LEGATO_MODEM_SERVICES}/platformAdaptor/inc
                        ${PA_SIMU_MODEM_SERVICES}
                        ${SIMU_CONFIG_TREE}
                        ${LEGATO_BUILD}/${TEST}
                        ${LEGATO_ROOT}/${TEST}
                        ${LEGATO_ROOT}/components/cfgEntries )

SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Dle_msg_AddServiceCloseHandler=le_msgSimu_AddServiceCloseHandler")

set(APP_SOURCES
    ${LEGATO_ROOT}/${TEST}/main.c
    ${LEGATO_MODEM_SERVICES}/modemDaemon/le_ecall.c
    ${LEGATO_MODEM_SERVICES}/modemDaemon/asn1Msd.c
    ${LEGATO_MODEM_SERVICES}/modemDaemon/le_mcc.c
    ${PA_SIMU_MODEM_SERVICES}/pa_ecall_simu.c
    ${PA_SIMU_MODEM_SERVICES}/pa_mcc_simu.c
    ${SIMU_CONFIG_TREE}/le_cfg_simu.c
)

set_legato_component(${APP_COMPONENT})
add_legato_executable(${APP_TARGET} ${APP_SOURCES})

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
