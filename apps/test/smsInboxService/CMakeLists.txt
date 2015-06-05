#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

mkapp(smsInboxTest.adef
        -i ${LEGATO_ROOT}/interfaces/smsInboxService
        -i ${LEGATO_ROOT}/interfaces/modemServices
        DEPENDS
            smsInboxTest/Component.cdef
            smsInboxTest/smsInboxTest.c
            legato
)
