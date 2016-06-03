#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(TEST_EXEC tempUnitTest)

set(INTERFACES_MODEM_SERVICES "${LEGATO_ROOT}/interfaces/modemServices/")
set(LEGATO_MODEM_SERVICES "${LEGATO_ROOT}/components/modemServices/")
set(PA_SIMU_MODEM_SERVICES "${MODEM_PA_DIR}/simu/components/le_pa/")
set (TEST "apps/test/modemServices/temp/${TEST_EXEC}")
set (LEGATO_BUILD "${LEGATO_ROOT}/build/${LEGATO_TARGET}")
set(LEGATO_LIBRARY_PATH ${LEGATO_ROOT}/build/${LEGATO_TARGET}/framework/lib/liblegato.so)

#generate API interfaces
execute_process (COMMAND ifgen --gen-interface ${INTERFACES_MODEM_SERVICES}/le_temp.api
                 WORKING_DIRECTORY ${LEGATO_BUILD}/${TEST})

include_directories(    ${LEGATO_ROOT}/framework/c/inc
                        ${LEGATO_ROOT}/framework/c/src
                        ${LEGATO_MODEM_SERVICES}/modemDaemon
                        ${LEGATO_MODEM_SERVICES}/platformAdaptor/inc
                        ${PA_SIMU_MODEM_SERVICES}
                        ${LEGATO_BUILD}/${TEST}
                        ${LEGATO_ROOT}/${TEST} )

add_executable(${TEST_EXEC} main.c
                            ${LEGATO_MODEM_SERVICES}/modemDaemon/le_temp.c
                            ${PA_SIMU_MODEM_SERVICES}/pa_temp_simu.c
                            )

target_link_libraries(${TEST_EXEC} ${LEGATO_LIBRARY_PATH})

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC} )
