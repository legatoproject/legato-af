#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_TARGET testModemSmsPdu)

message("CUNIT_INCLUDE_DIRS: ${CUNIT_INCLUDE_DIRS}")

mkexe(${APP_TARGET}
            smsPduTest
            ${CUNIT_LIBRARIES}
            -i ${CUNIT_INSTALL}/include
            -i ${CUNIT_INSTALL}/include/CUnit
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/components/modemServices/modemDaemon
            -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            smsPduTest/Component.cdef
            smsPduTest/smsPduTest.c
            ${LEGATO_ROOT}/components/modemServices/modemDaemon/smsPdu.c
            ${LEGATO_ROOT}/components/modemServices/modemDaemon/cdmaPdu.c
            )


add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
