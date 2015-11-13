#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(TEST_EXEC ecallUnitTest)

set(INTERFACES "${LEGATO_ROOT}/interfaces")
set(INTERFACES_MODEM_SERVICES "${LEGATO_ROOT}/interfaces/modemServices/")
set(LEGATO_MODEM_SERVICES "${LEGATO_ROOT}/components/modemServices/")
set(PA_SIMU_MODEM_SERVICES "${LEGATO_MODEM_SERVICES}/platformAdaptor/simu/le_pa/")
set(SIMU_CONFIG_TREE "${CMAKE_CURRENT_SOURCE_DIR}/simu/")
set (TEST "apps/test/modemServices/ecall/${TEST_EXEC}")
set (LEGATO_BUILD "${LEGATO_ROOT}/build/${LEGATO_TARGET}")

#generate API interfaces
execute_process (COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_ecall.api
                 COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_mcc.api
                 COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_mdmDefs.api
                 COMMAND ifgen --gen-interface ${INTERFACES}/le_cfg.api
                 COMMAND ifgen --gen-interface ${INTERFACES}/le_cfgAdmin.api
                 COMMAND ifgen --gen-interface ${INTERFACES}/powerMgr/le_pm.api
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

add_executable(${TEST_EXEC} main.c
               ${LEGATO_MODEM_SERVICES}/modemDaemon/le_ecall.c
               ${LEGATO_MODEM_SERVICES}/modemDaemon/asn1Msd.c
               ${LEGATO_MODEM_SERVICES}/modemDaemon/le_mcc.c
               ${PA_SIMU_MODEM_SERVICES}/pa_ecall_simu.c
               ${PA_SIMU_MODEM_SERVICES}/pa_mcc_simu.c
               ${SIMU_CONFIG_TREE}/le_cfg_simu.c
              )

target_link_libraries(${TEST_EXEC} legato)


add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC} )
