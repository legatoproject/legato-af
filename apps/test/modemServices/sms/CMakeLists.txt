#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_TARGET testModemSms)

message("CUNIT_INCLUDE_DIRS: ${CUNIT_INCLUDE_DIRS}")

## TODO: Remove this when the mk tools do dependency checking.
if(EXISTS ${LEGATO_ROOT}/components/modemServices/platformAdaptor/qmi/le_pa/pa_sms_qmi.c)

    set(PA_SOURCES
             ${LEGATO_ROOT}/components/modemServices/platformAdaptor/qmi/le_pa/pa_sms_qmi.c
        )

endif()


mkexe(${APP_TARGET}
            smsTest
            ${CUNIT_LIBRARIES}
            -i ${CUNIT_INSTALL}/include
            -i ${CUNIT_INSTALL}/include/CUnit
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
        DEPENDS
            ## TODO: Remove this when the mk tools do dependency checking.
            smsTest/Component.cdef
            smsTest/main.c
            smsTest/main.h
            smsTest/le_smsTest.c
            ${PA_SOURCES}
            )

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()


set(APP_TARGET testModemSmsDeletion)

mkexe(${APP_TARGET}
            smsDeletion
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
        DEPENDS
            ## TODO: Remove this when the mk tools do dependency checking.
            smsDeletion/Component.cdef
            smsDeletion/smsDeletion.c
            ${PA_SOURCES}
            )

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()