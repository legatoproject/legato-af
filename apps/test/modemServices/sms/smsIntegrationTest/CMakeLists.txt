#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

mkapp(smsTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
)

mkapp(smsDeletion.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
)

mkapp(smsCBTest.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
)
