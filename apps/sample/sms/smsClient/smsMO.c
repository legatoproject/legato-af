 /**
  * Sample code for Mobile Originated SMS messages.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  *
  *
  */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function simply sends a text message.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsmo_SendMessage
(
    const char*   destinationPtr, ///< [IN] The destination number.
    const char*   textPtr         ///< [IN] The SMS message text.
)
{
    le_result_t           res;
    le_sms_MsgRef_t      myMsg;

    myMsg = le_sms_Create();
    if (!myMsg)
    {
        LE_ERROR("SMS message creation has failed!");
        return LE_FAULT;
    }

    res = le_sms_SetDestination(myMsg, destinationPtr);
    if (res != LE_OK)
    {
        LE_ERROR("le_sms_SetDestination has failed (res.%d)!", res);
        return LE_FAULT;
    }

    res = le_sms_SetText(myMsg, textPtr);
    if (res != LE_OK)
    {
        LE_ERROR("le_sms_SetText has failed (res.%d)!", res);
        return LE_FAULT;
    }

    res = le_sms_Send(myMsg);
    if (res != LE_OK)
    {
        LE_ERROR("le_sms_Send has failed (res.%d)!", res);
        return LE_FAULT;
    }
    else
    {
        LE_INFO("\"%s\" has been successfully sent to %s.", textPtr, destinationPtr);
    }

    le_sms_Delete(myMsg);

    return LE_OK;
}

