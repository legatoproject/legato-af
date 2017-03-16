#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Creates application from the sms.adef
mkapp(sms.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
)

# This is a sample application
add_dependencies(samples_c sms)
