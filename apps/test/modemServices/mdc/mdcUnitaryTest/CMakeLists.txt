#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_TARGET testModemMdcUnitaryTest)

if(${LEGATO_TARGET} STREQUAL "localhost")

    mkexe(${APP_TARGET}
            mdcUnitaryTest
            -C "-g"
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
            -i ${LEGATO_ROOT}/components/modemServices/modemDaemon
            -i ${LEGATO_ROOT}/proprietary/qct/inc/framework
            -i ${LEGATO_ROOT}/proprietary/qct/inc/services

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
