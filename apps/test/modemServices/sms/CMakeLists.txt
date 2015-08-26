#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

mkapp(smsTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    DEPENDS
        legato
        smsTest/Component.cdef
        smsTest/le_smsTest.c
)

mkapp(smsDeletion.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    DEPENDS
        legato
        smsDeletion/Component.cdef
        smsDeletion/smsDeletion.c
)

mkapp(smsCBTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    DEPENDS
        legato
        smsCBTest/Component.cdef
        smsCBTest/smsCBTest.c
)
