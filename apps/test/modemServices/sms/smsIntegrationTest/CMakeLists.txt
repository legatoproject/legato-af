#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
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
