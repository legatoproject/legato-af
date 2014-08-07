#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc.
#
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

# Creates application from the sms.adef
mkapp(sms.adef
    -i ${LEGATO_ROOT}/interfaces/modemServices
    DEPENDS
        smsClient/Component.cdef
        smsClient/main.c
        smsClient/smsMO.c
        smsClient/smsMT.c
        smsClient/smsSample.h
        legato
)

# This is a sample application
add_dependencies(samples sms)
