#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_TARGET testModemSmsCdmaPdu)

message("CUNIT_INCLUDE_DIRS: ${CUNIT_INCLUDE_DIRS}")

mkexe(${APP_TARGET}
            cdmaPduTest
            ${CUNIT_LIBRARIES}
            -i ${CUNIT_INSTALL}/include
            -i ${CUNIT_INSTALL}/include/CUnit
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/components/modemServices/modemDaemon
            -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            cdmaPduTest/Component.cdef
            cdmaPduTest/cdmaPduTest.c
            ${LEGATO_ROOT}/components/modemServices/modemDaemon/cdmaPdu.c
            )

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)