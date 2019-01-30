#include "legato.h"
#include "interfaces.h"
#include "smsSample.h"

//--------------------------------------------------------------------------------------------------
/**
 * References
 */
//--------------------------------------------------------------------------------------------------
static le_sms_MsgListRef_t MsgListHandler;
static le_sms_MsgRef_t currentMsgRef;

//-------------------------------------------------------------------------------------------------
/**
 * Check whether the user provided number is valid!
 */
//-------------------------------------------------------------------------------------------------
static le_result_t isNumValid
(
    const char* phoneNumber
)
{
    if (NULL == phoneNumber)
    {
        LE_ERROR("Phone number NULL");
        return LE_FAULT;
    }
    int i = 0;
    int numLength = strlen(phoneNumber);
    if (numLength+1 > LE_MDMDEFS_PHONE_NUM_MAX_BYTES)
    {
        LE_INFO("The number is too long!");
        return LE_FAULT;
    }
    for (i = 0; i <= numLength-1; i++)
    {
        char dig = *phoneNumber;
        if(!isdigit(dig))
        {
            LE_INFO("The input contains non-digit symbol %c", dig);
            return LE_FAULT;
        }
        phoneNumber++;
    }
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the first message from the message list and display the contents
 */
//-------------------------------------------------------------------------------------------------
static le_result_t displayFirst
(
    le_sms_MsgListRef_t MsgHandler
)
{
    le_result_t res = LE_OK;
    char text[LE_SMS_TEXT_MAX_BYTES] = {0};

    currentMsgRef = le_sms_GetFirst(MsgHandler);
    if (currentMsgRef == NULL)
    {
        LE_ERROR("No message found!!");
        res = LE_FAULT;
    }
    else
    {
        LE_INFO("Message found!");
        res = le_sms_GetText(currentMsgRef, text, sizeof(text));
        if(res != LE_OK)
        {
          LE_ERROR("le_sms_GetText has failed (res.%d)!", res);
        }
        else
        {
          LE_INFO("Message content: \"%s\"", text);
        }
    }
    return res;
}

//-------------------------------------------------------------------------------------------------
/**
* Get the phone number specified on the command line followed by a text message and send it.
* By default the message is sent synchronously. If sendAsync is specified message will be sent
* asynchronously.
*
* @return
*      - LE_OK              If message is successfully sent to the destination number.
*      - LE_FAULT           If message cannot be sent to the destination number.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlSMS_SendMessage
(
    const char * numPtr,
    const char * txtPtr,
    bool sendAsync,
    bool sendAt
)
{
    le_result_t res;
    const char* message_to_send = txtPtr;
    const char* phoneNumber = numPtr;
    char  textMessage[LE_SMS_TEXT_MAX_BYTES] = {0};
    char destinationNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    int msgParts = 0;
    int msgLength = strlen(message_to_send);

    if(isNumValid(phoneNumber) != LE_OK)
    {
      LE_INFO("Phone number is not valid!");
      res = LE_FAULT;
    }
    else
    {
      strncpy(destinationNumber, phoneNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
      LE_INFO("Phone number %s", destinationNumber);
    }

    if (msgLength >= LE_SMS_TEXT_MAX_BYTES)
    {
        // Divide text by LE_SMS_TEXT_MAX_BYTES - 1 to get number of text messages required to send the full text
        msgParts = msgLength / (LE_SMS_TEXT_MAX_BYTES - 1);
        LE_INFO("Text message too large to send in one message. Breaking it up to %d messages", msgParts + 1); // plus 1 due to remainder of text
    }

    // loop over entire text and send LE_SMS_TEXT_MAX_BYTES characters at a time
    int i;
    for (i = 0; i <= msgParts; i++)
    {
        snprintf(textMessage, sizeof(textMessage), message_to_send);
        if (sendAt)
        {
          res = smsmo_SendMessageAT(phoneNumber, textMessage);
          if (res != LE_OK)
          {
              LE_ERROR("Failed to send message using AT commands (res.%d)!", res);
          }
          else
          {
              LE_INFO("Message sent using AT commands.");
          }
        }
        else if (!sendAsync)
        {
          res = smsmo_SendMessage(destinationNumber, textMessage, false);
          if (res != LE_OK)
          {
              LE_ERROR("Failed to send message synchronously (res.%d)!", res);
          }
          else
          {
              LE_INFO("Message sent synchronously.");
          }
        }
        else
        {
          res = smsmo_SendMessage(destinationNumber, textMessage, true);
          if (res != LE_OK)
          {
              LE_ERROR("Failed to send message asynchronously (res.%d)!", res);
          }
          else
          {
              LE_INFO("Message sent asynchronously.");
          }
        }
        // increment pointer by LE_SMS_TEXT_MAX_BYTES-1 for next iteration
        message_to_send += (LE_SMS_TEXT_MAX_BYTES-1);
    }

    return res;
}

//-------------------------------------------------------------------------------------------------
/**
* Creates a message list containing all the messages stored on the device and displays the first
* message in the list.
*
* @return
*      - LE_OK              If message list is successfully created and first message is displayed.
*      - LE_FAULT           If message list is not created or first message cannot be displayed.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlSMS_GetInbox
(
    void
)
{
    le_result_t res = LE_OK;
    le_sms_MsgRef_t msgPtr;
    int inboxCount = 0;

    MsgListHandler = le_sms_CreateRxMsgList();
    if (MsgListHandler == NULL)
    {
      LE_ERROR("Inbox empty!");
      return LE_OK;
    }

    msgPtr = le_sms_GetFirst(MsgListHandler);
    while(msgPtr != NULL)
    {
      msgPtr = le_sms_GetNext(MsgListHandler);
      inboxCount++;
    }
    LE_INFO("There are %d messages in your inbox!", inboxCount);
    res = displayFirst(MsgListHandler);
    if (res != LE_OK)
    {
      LE_ERROR("Message cannot be displayed!");
    }
    return res;
}

//-------------------------------------------------------------------------------------------------
/**
* Displays the next message in the list after message list has already been created using Inbox.
* If end of the list is reached, it will go back to the beginning of the list.
*
* @return
*      - LE_OK              If next message is successfully displayed.
*      - LE_FAULT           If next message cannot be displayed.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlSMS_GetNext
(
    void
)
{
    le_result_t res = LE_OK;
    char text[LE_SMS_TEXT_MAX_BYTES] = {0};

    if (MsgListHandler == NULL)
    {
        LE_ERROR("Please bring up the inbox first by typing -> sms inbox, then see subsequent messages by performing -> sms next");
        return LE_NOT_FOUND;
    }

    currentMsgRef = le_sms_GetNext(MsgListHandler);
    if (currentMsgRef == NULL)
    {
        LE_INFO("End of messages. Going back to first message.");
        le_sms_DeleteList(MsgListHandler);
        MsgListHandler = le_sms_CreateRxMsgList();

        if (MsgListHandler == NULL)
        {
          LE_ERROR("Inbox empty!");
          return LE_OK;
        }
        res = displayFirst(MsgListHandler);
    }
    else
    {
        LE_INFO("Message found!");
        res = le_sms_GetText(currentMsgRef, text, sizeof(text));
        if(res != LE_OK)
        {
          LE_ERROR("le_sms_GetText has failed (res.%d)!", res);
        }
        else
        {
          LE_INFO("Message content: \"%s\"", text);
        }
    }
    return res;
}

//-------------------------------------------------------------------------------------------------
/**
* Deletes from storage the last message which was displayed using either Inbox or Next.
*
* @return
*      - LE_OK              If message is successfully deleted.
*      - LE_NO_MEMORY       If the message is not present in storage area.
*      - LE_FAULT           If message cannot be deleted or message list hasn't been created yet.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlSMS_DeleteMessage
(
    void
)
{
    le_result_t res = LE_OK;

    if (MsgListHandler == NULL)
    {
        LE_ERROR("Please bring up the inbox first by typing -> sms inbox, delete a message by typing -> sms delete");
        return LE_NOT_FOUND;
    }

    res = le_sms_DeleteFromStorage(currentMsgRef);

    if(res == LE_NO_MEMORY)
    {
      LE_ERROR("The message is not present in storage area (res.%d)!", res);
    }
    else if(res == LE_FAULT)
    {
      LE_ERROR("Failed to delete message from storage (res.%d)!", res);
    }
    else
    {
      LE_INFO("Successfully deleted the message");
    }
    return res;
}

//-------------------------------------------------------------------------------------------------
/**
* Displays the status of the last message which was displayed using either Inbox or Next.
*
* @return
*      - LE_OK              If message status is successfully displayed.
*      - LE_FAULT           If message status cannot be displayed.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlSMS_GetStatus
(
    void
)
{
    le_result_t res = LE_OK;
    le_sms_Status_t currentMsgStatus;

    if (MsgListHandler == NULL)
    {
        LE_ERROR("Please bring up the inbox first by typing -> sms inbox, then see the status of a message by typing -> sms status");
        return LE_NOT_FOUND;
    }

    currentMsgStatus = le_sms_GetStatus(currentMsgRef);

    if(currentMsgStatus == LE_SMS_RX_READ)
    {
      LE_INFO("Message present in the message storage has been read.");
      res = LE_OK;
    }
    else if(currentMsgStatus == LE_SMS_RX_UNREAD)
    {
      LE_INFO("Message present in the message storage has not been read.");
      res = LE_OK;
    }
    return res;
}

//-------------------------------------------------------------------------------------------------
/**
* Counts the total number of received messages since the last reset count.
*
* @return
*      - LE_OK              If the count is successfully displayed.
*      - LE_FAULT           If message count cannot be displayed.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlSMS_GetCount
(
    void
)
{
    le_result_t res = LE_OK;
    le_sms_Type_t msgType = LE_SMS_TYPE_RX;
    int32_t messageCount;
    res = le_sms_GetCount(msgType, &messageCount);

    if(res == LE_BAD_PARAMETER)
    {
      LE_INFO("LE_BAD_PARAMETER received");
      res = LE_OK;
    }
    else
    {
      LE_INFO("The number of received messages is: %d", messageCount);
      res = LE_OK;
    }
    return res;
}

//-------------------------------------------------------------------------------------------------
/**
* Marks as unread, the last message which was displayed using either Inbox or Next.
*
* @return
*      - LE_OK              If message is successfully marked as unread.
*      - LE_FAULT           If message cannot be marked as unread.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlSMS_MarkUnread
(
    void
)
{
    if (MsgListHandler == NULL)
    {
        LE_ERROR("Please bring up the inbox first by typing -> sms inbox, then mark a message as unread by typing -> sms unread");
        return LE_NOT_FOUND;
    }
    le_sms_MarkUnread(currentMsgRef);
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
* Resets the received message counter.
*
* @return
*      - LE_OK              If the counter was successfully reset.
*      - LE_FAULT           If message counter cannot be reset.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlSMS_ResetCount
(
    void
)
{
    if (MsgListHandler == NULL)
    {
        LE_ERROR("Please bring up the inbox first by typing -> sms inbox, then reset the received message counter by typing -> sms reset");
        return LE_NOT_FOUND;
    }
    le_sms_ResetCount();
    return LE_OK;
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

    smsmt_Receiver();
    smsmt_MonitorStorage();
}
