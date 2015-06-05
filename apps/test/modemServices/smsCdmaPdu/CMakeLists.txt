#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

mkapp(cdmaPduTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_ROOT}/components/modemServices/modemDaemon
    -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
    DEPENDS
        legato
        cdmaPduTest/Component.cdef
        cdmaPduTest/cdmaPduTest.c
       ${LEGATO_ROOT}/components/modemServices/modemDaemon/smsPdu.c
       ${LEGATO_ROOT}/components/modemServices/modemDaemon/cdmaPdu.c
)
