// -------------------------------------------------------------------------------------------------
/**
 *  SMS Inbox Server
 *
 *  message box definition.
 *
 *  Copyright (C) Sierra Wireless Inc.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "le_smsInbox.h"


//--------------------------------------------------------------------------------------------------
/**
 * message box names to be started.
 * Must be consistent with the message boxes names provided into the Component.cdef
 *
 */
//--------------------------------------------------------------------------------------------------
const char* le_smsInbox_mboxName[]=
{
    "le_smsInbox1",
    "le_smsInbox2",
};

//--------------------------------------------------------------------------------------------------
/**
 * message boxes Number.
 *
 */
//--------------------------------------------------------------------------------------------------
const uint8_t le_smsInbox_NbMbx = sizeof(le_smsInbox_mboxName)/sizeof(char*);

//--------------------------------------------------------------------------------------------------
/**
 * Create the smsInbox wrappers.
 * APIs are created here, according to the message boxes names provided into the Component.cdef
 *
 */
//--------------------------------------------------------------------------------------------------
DEFINE_MBX(le_smsInbox1)
DEFINE_MBX(le_smsInbox2)
