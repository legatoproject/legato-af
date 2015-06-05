#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

mkapp(smsPduTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    -i ${LEGATO_ROOT}/components/modemServices/modemDaemon
    -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
    DEPENDS
        legato
        smsPduTest/Component.cdef
        smsPduTest/smsPduTest.c
       ${LEGATO_ROOT}/components/modemServices/modemDaemon/smsPdu.c
       ${LEGATO_ROOT}/components/modemServices/modemDaemon/cdmaPdu.c
)
