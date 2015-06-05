#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)


mkapp(mdcUnitaryTest.adef
        -C "-g -Dle_msg_AddServiceCloseHandler=stub_le_msg_AddServiceCloseHandler"
        -i ${LEGATO_ROOT}/interfaces/modemServices
        -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
        -i ${LEGATO_ROOT}/components/modemServices/modemDaemon
        -i ${LEGATO_ROOT}/components/modemServices/modemDaemon
        -i ${LEGATO_ROOT}/proprietary/qct/inc/framework
        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            mdcUnitaryTestComp/Component.cdef
            mdcUnitaryTestComp/mdcUnitaryTest.c
)

