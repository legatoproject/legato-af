 /**
  * Main functions to use the SMS sample codes.
  *
  * Copyright (C) Sierra Wireless, Inc. 2014.
  *
  * Use of this work is subject to license.
  *
  */

#include "legato.h"
#include "interfaces.h"
#include "smsSample.h"


//--------------------------------------------------------------------------------------------------
/**
 * The thread to install the SMS message handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* SMSReceiver
(
    void* context
)
{
    smsmt_Receiver();

    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 *  App init
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start SMS Sample!");

    le_thread_Start(le_thread_Create("SMSReceiver", SMSReceiver, NULL));
}
