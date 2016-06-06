#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(TEST_EXEC smsUnitTest)

set(LEGATO_MODEM_SERVICES "${LEGATO_ROOT}/components/modemServices")

mkexe(${TEST_EXEC}
    .
    -i ${LEGATO_MODEM_SERVICES}/modemDaemon
    -i ${LEGATO_MODEM_SERVICES}/platformAdaptor/inc
    -i ${LEGATO_ROOT}/components/cfgEntries
    -i ${LEGATO_ROOT}/framework/c/src
    -i ${PA_DIR}/simu/components/le_pa
)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC})
