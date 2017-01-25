 /**
  * Sample code for Mobile Terminated SMS messages.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  *
  *
  */

#include "legato.h"
#include "interfaces.h"
#include "smsSample.h"

#define  MESSAGE_FEEDBACK "Message from %s received"

static le_sms_RxMessageHandlerRef_t RxHdlrRef;
static le_sms_FullStorageEventHandlerRef_t FullStorageHdlrRef;


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS message reception.
 *
 */
//--------------------- -----------------------------------------------------------------------------
static void RxMessageHandler
(
    le_sms_MsgRef_t msgRef,
    void*            contextPtr
)
{
    le_result_t  res;
    char         tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    char         timestamp[LE_SMS_TIMESTAMP_MAX_BYTES] = {0};
    char         text[LE_SMS_TEXT_MAX_BYTES] = {0};
    char         textReturn[LE_SMS_TEXT_MAX_BYTES] = {0};

    LE_INFO("A New SMS message is received with ref.%p", msgRef);

    if (le_sms_GetFormat(msgRef) == LE_SMS_FORMAT_TEXT)
    {
        res = le_sms_GetSenderTel(msgRef, tel, sizeof(tel));
        if(res != LE_OK)
        {
            LE_ERROR("le_sms_GetSenderTel has failed (res.%d)!", res);
        }
        else
        {
            LE_INFO("Message is received from %s.", tel);
        }

        res = le_sms_GetTimeStamp(msgRef, timestamp, sizeof(timestamp));
        if(res != LE_OK)
        {
            LE_ERROR("le_sms_GetTimeStamp has failed (res.%d)!", res);
        }
        else
        {
            LE_INFO("Message timestamp is %s.", timestamp);
        }

        res = le_sms_GetText(msgRef, text, sizeof(text));
        if(res != LE_OK)
        {
            LE_ERROR("le_sms_GetText has failed (res.%d)!", res);
        }
        else
        {
            LE_INFO("Message content: \"%s\"", text);
        }

        snprintf(textReturn, sizeof(textReturn), MESSAGE_FEEDBACK, tel);

        // Return a message to sender with phone number include (see smsMO.c file)
        res = smsmo_SendMessage(tel, textReturn);
        if (res != LE_OK)
        {
            LE_ERROR("SmsMoMessage has failed (res.%d)!", res);
        }
        else
        {
            LE_INFO("the message has been successfully sent.");
        }

        res = le_sms_DeleteFromStorage(msgRef);
        if(res != LE_OK)
        {
            LE_ERROR("le_sms_DeleteFromStorage has failed (res.%d)!", res);
        }
        else
        {
            LE_INFO("the message has been successfully deleted from storage.");
        }
    }
    else
    {
        LE_WARN("Warning! I read only Text messages!");
    }

    le_sms_Delete(msgRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS storage full message indication.
 *
 */
//--------------------- -----------------------------------------------------------------------------
static void StorageMessageHandler
(
    le_sms_Storage_t  storage,
    void*            contextPtr
)
{
    LE_INFO("A Full storage SMS message is received. Type of full storage %d", storage);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function installs an handler for message reception.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsmt_Receiver
(
    void
)
{
    RxHdlrRef = le_sms_AddRxMessageHandler(RxMessageHandler, NULL);
    if (!RxHdlrRef)
    {
        LE_ERROR("le_sms_AddRxMessageHandler has failed!");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function installs an handler for storage message indication.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsmt_MonitorStorage
(
    void
)
{
    FullStorageHdlrRef = le_sms_AddFullStorageEventHandler(StorageMessageHandler, NULL);
    if (!FullStorageHdlrRef)
    {
        LE_ERROR("le_sms_AddFullStorageEventHandler has failed!");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function remove the handler for message reception.
 *
 */
//--------------------------------------------------------------------------------------------------
void smsmt_HandlerRemover
(
    void
)
{
    le_sms_RemoveRxMessageHandler(RxHdlrRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function remove the handler for storage message indication.
 *
 */
//--------------------------------------------------------------------------------------------------
void smsmt_StorageHandlerRemover
(
    void
)
{
    le_sms_RemoveFullStorageEventHandler(FullStorageHdlrRef);
}
