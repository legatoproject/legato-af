#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

set(SMS_INBOX_COMPONENT_DIR ${LEGATO_ROOT}/components/smsInboxService)
set(JANSSON_INC_DIR "${CMAKE_BINARY_DIR}/3rdParty/jansson-2.6/include/")

mkapp(  smsInboxService.adef
            -i ${LEGATO_ROOT}/interfaces/smsInboxService
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/components/cfgEntries
            -i ${LEGATO_ROOT}/components/smsInboxService
            -i ${LEGATO_ROOT}/components/smsInboxService/smsInbox
            -i ${JANSSON_INC_DIR}
            --ldflags=-ljansson
            --cflags="-g"
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${SMS_INBOX_COMPONENT_DIR}/Component.cdef
            ${SMS_INBOX_COMPONENT_DIR}/smsInbox.c
            ${SMS_INBOX_COMPONENT_DIR}/Component.cdef
            ${SMS_INBOX_COMPONENT_DIR}/le_smsInbox.c
            ${LEGATO_ROOT}/interfaces/smsInboxService/le_smsInbox1.api
            ${LEGATO_ROOT}/interfaces/modemServices/le_sim.api
            ${LEGATO_ROOT}/interfaces/modemServices/le_sms.api
            ${LEGATO_ROOT}/interfaces/le_cfg.api
            ${LEGATO_ROOT}/components/cfgEntries/mdmCfgEntries.h
        )

add_dependencies(smsInboxService jansson)
