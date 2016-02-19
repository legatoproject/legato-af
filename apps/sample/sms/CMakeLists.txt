#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

# Creates application from the sms.adef
mkapp(sms.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
)

# This is a sample application
add_dependencies(samples sms)
