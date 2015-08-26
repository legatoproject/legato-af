#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(TEST_EXEC mdcUnitTest)

set(JANSSON_INC_DIR "${CMAKE_BINARY_DIR}/3rdParty/jansson-2.6/include/")
set(INTERFACES_MODEM_SERVICES "${LEGATO_ROOT}/interfaces/modemServices/")
set(LEGATO_MODEM_SERVICES "${LEGATO_ROOT}/components/modemServices/")
set(PA_SIMU_MODEM_SERVICES "${LEGATO_MODEM_SERVICES}/platformAdaptor/simu/le_pa/")
set (TEST "apps/test/modemServices/mdc/${TEST_EXEC}")
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

add_executable(${TEST_EXEC} main.c
                            ${LEGATO_MODEM_SERVICES}/modemDaemon/le_mdc.c
                            ${LEGATO_MODEM_SERVICES}/modemDaemon/le_mrc.c
                            ${LEGATO_MODEM_SERVICES}/modemDaemon/le_sim.c
                            ${PA_SIMU_MODEM_SERVICES}/pa_mrc_simu.c
                            ${PA_SIMU_MODEM_SERVICES}/pa_mdc_simu.c
                            ${PA_SIMU_MODEM_SERVICES}/pa_sim_simu.c
                            )


target_link_libraries(${TEST_EXEC} legato jansson)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC} ${APNFILE} )
