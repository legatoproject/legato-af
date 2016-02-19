#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(APP_COMPONENT mrcTest)
set(APP_TARGET mrcUnitTest)

set(INTERFACES "${LEGATO_ROOT}/interfaces")
set(INTERFACES_MODEM_SERVICES "${LEGATO_ROOT}/interfaces/modemServices/")
set(LEGATO_MODEM_SERVICES "${LEGATO_ROOT}/components/modemServices/")
set(PA_SIMU_MODEM_SERVICES "${LEGATO_MODEM_SERVICES}/platformAdaptor/simu/le_pa/")
set (TEST "apps/test/modemServices/mrc/${APP_TARGET}")
set (LEGATO_BUILD "${LEGATO_ROOT}/build/${LEGATO_TARGET}")

#generate API interfaces
execute_process (COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_mdmDefs.api
                 COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_sim.api
                 COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_mrc.api
                 WORKING_DIRECTORY ${LEGATO_BUILD}/${TEST})

include_directories(    ${LEGATO_ROOT}/framework/c/inc
                        ${LEGATO_ROOT}/framework/c/src
                        ${LEGATO_MODEM_SERVICES}/modemDaemon
                        ${LEGATO_MODEM_SERVICES}/platformAdaptor/inc
                        ${PA_SIMU_MODEM_SERVICES}
                        ${LEGATO_BUILD}/${TEST}
                        ${LEGATO_ROOT}/${TEST}
                        ${LEGATO_ROOT}/components/cfgEntries )

set(APP_SOURCES
    ${LEGATO_ROOT}/${TEST}/main.c
    ${LEGATO_MODEM_SERVICES}/modemDaemon/le_mrc.c
    ${LEGATO_MODEM_SERVICES}/modemDaemon/le_sim.c
    ${PA_SIMU_MODEM_SERVICES}/pa_mrc_simu.c
    ${PA_SIMU_MODEM_SERVICES}/pa_sim_simu.c
)

set_legato_component(${APP_COMPONENT})
add_legato_executable(${APP_TARGET} ${APP_SOURCES})

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
