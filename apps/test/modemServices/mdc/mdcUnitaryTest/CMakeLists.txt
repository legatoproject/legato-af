#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_TARGET testModemMdcUnitaryTest)

if(${LEGATO_TARGET} STREQUAL "localhost")

    mkexe(${APP_TARGET}
            mdcUnitaryTest
            -C "-g -Dle_msg_AddServiceCloseHandler=stub_le_msg_AddServiceCloseHandler"
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
            -i ${LEGATO_ROOT}/components/modemServices/modemDaemon
            -i ${LEGATO_ROOT}/components/modemServices/modemDaemon
            -i ${LEGATO_ROOT}/proprietary/qct/inc/framework

        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            mdcUnitaryTest/Component.cdef
            mdcUnitaryTest/mdcUnitaryTest.c
            ${LEGATO_ROOT}/components/modemServices/platformAdaptor/stub/le_pa/pa_mdc_stub.c
            )

    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()
