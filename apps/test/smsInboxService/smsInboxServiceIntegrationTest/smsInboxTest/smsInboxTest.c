/**
 * This module implements the SMS Inbox Service test.
 *
* You must issue the following commands:
* @verbatim
  $ app start smsInboxTest
  $ app runProc smsInboxTest --exe=smsInboxTest -- <read/receive>
 @endverbatim
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum message count for SMS Inbox.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_MESSAGE_INVALID_COUNT 101
#define MAX_MESSAGE_VALID_COUNT   10

static le_smsInbox1_RxMessageHandlerRef_t  HandlerRef = NULL;

static le_smsInbox1_SessionRef_t MyMbx1Ref = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Test: Get message details
 *
 */
//--------------------------------------------------------------------------------------------------
void Testmbx_GetDetails
(
    uint32_t    msgId
)
{
    char     imsi[LE_SIM_IMSI_BYTES];
    char     tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    char     timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    char     text[LE_SMS_TEXT_MAX_BYTES] = {0};
    uint8_t  bin[LE_SMS_BINARY_MAX_BYTES] = {0};
    uint8_t  pdu[LE_SMS_PDU_MAX_BYTES] = {0};
    size_t   uintval;
    le_sms_Format_t format;

    LE_ASSERT(le_smsInbox1_GetImsi( msgId, imsi, 1) == LE_OVERFLOW);
    LE_ASSERT(le_smsInbox1_GetImsi( msgId, imsi, sizeof(imsi)) == LE_OK);
    LE_INFO("IMSI tied to the message is \"%s\".", imsi);

    uintval = le_smsInbox1_GetMsgLen( msgId);
    LE_INFO("Length of the message is %zd.", uintval);

    format = le_smsInbox1_GetFormat( msgId);
    LE_INFO("SMS format is is %d.", format);

    switch (format)
    {
        case LE_SMS_FORMAT_TEXT:
        case LE_SMS_FORMAT_BINARY:
        {
            LE_ASSERT(le_smsInbox1_GetSenderTel( msgId, tel, 1) == LE_OVERFLOW);
            LE_ASSERT(le_smsInbox1_GetSenderTel(msgId, tel, sizeof(tel)) == LE_OK);
            LE_INFO("Sender telephone of the message is \"%s\".", tel);

            LE_ASSERT(le_smsInbox1_GetTimeStamp(msgId, timestamp, 1) == LE_OVERFLOW);
            LE_ASSERT(le_smsInbox1_GetTimeStamp(msgId, timestamp, sizeof(timestamp)) ==
                                                                                            LE_OK);
            LE_INFO("Timestamp of the message is \"%s\".", timestamp);

            if (uintval)
            {
                switch (format)
                {
                    case LE_SMS_FORMAT_TEXT:
                        memset(text,0,sizeof(text));
                        LE_ASSERT(le_smsInbox1_GetText(msgId, text, 1) == LE_OVERFLOW);
                        LE_ASSERT(le_smsInbox1_GetText(msgId, text, sizeof(text)) == LE_OK);
                        LE_INFO("Content of the TEXT message is \"%s\".", text);
                    break;
                    case LE_SMS_FORMAT_BINARY:
                        uintval = 0;
                        LE_ASSERT(le_smsInbox1_GetBinary(msgId, bin, &uintval) ==
                                                                                    LE_OVERFLOW);
                        uintval = sizeof(bin);
                        LE_ASSERT(le_smsInbox1_GetBinary(msgId, bin, &uintval) == LE_OK);
                        LE_INFO("Content of the BINARY message is 0x%x 0x%x 0x%x ... (length.%d)",
                                bin[0], bin[1], bin[2], (int) uintval);
                    break;
                    default:
                    break;
                }
            }
        }
        break;

        case LE_SMS_FORMAT_PDU:
            if (uintval)
            {
                uintval = 1;
                LE_ASSERT(le_smsInbox1_GetPdu( msgId, pdu, &uintval) == LE_OVERFLOW);
                uintval = sizeof(pdu);
                LE_ASSERT(le_smsInbox1_GetPdu( msgId, pdu, &uintval) == LE_OK);
                LE_INFO("Content of the PDU message is 0x%x 0x%x 0x%x ... (length.%d)",
                        pdu[0], pdu[1], pdu[2], (int) uintval);
            }
        break;

        case LE_SMS_FORMAT_UNKNOWN:
        default:
            LE_ERROR("Unknown message format!");
        break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Read/Unread status
 *
 */
//--------------------------------------------------------------------------------------------------
void Testmbx_MsgStatus
(
    uint32_t msgId
)
{
    LE_INFO("Message is marked as %s", (le_smsInbox1_IsUnread(msgId)) ? "Unread":"Read");
    le_smsInbox1_MarkUnread(msgId);
    LE_ASSERT(le_smsInbox1_IsUnread(msgId) == true);
    le_smsInbox1_MarkRead(msgId);
    LE_ASSERT(le_smsInbox1_IsUnread(msgId) == false);
}

//--------------------------------------------------------------------------------------------------
/**
 * Rx Message Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestRxMsgHandler
(
    uint32_t msgId,
    void*    contextPtr
)
{
    LE_INFO("New received message!");

    Testmbx_GetDetails(msgId);

    le_smsInbox1_DeleteMsg(msgId);

    char     imsi[LE_SIM_IMSI_BYTES];
    LE_ASSERT(le_smsInbox1_GetImsi(msgId, imsi, sizeof(imsi)) != LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Read all received messages
 *
 */
//--------------------------------------------------------------------------------------------------
void Testmbx_GetMessages
(
    void
)
{
    uint32_t msgId;
    uint32_t             i=1;

    LE_INFO("Start Testmbx_GetMessages");

    msgId = le_smsInbox1_GetFirst(MyMbx1Ref);
    if (msgId != 0)
    {
        LE_INFO("Get message #%d", i);

        Testmbx_GetDetails(msgId);
        Testmbx_MsgStatus(msgId);
    }
    else
    {
        LE_INFO("There is no message in my Inbox folder!");
        return ;
    }

    while((msgId=le_smsInbox1_GetNext(MyMbx1Ref)) != 0)
    {
        i++;
        LE_INFO("Get message #%d", i);
        Testmbx_GetDetails(msgId);
        Testmbx_MsgStatus(msgId);
    }

    LE_INFO("End Testmbx_GetMessages");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Install Rx Message handler
 *
 */
//--------------------------------------------------------------------------------------------------
void Testmbx_AddRxMessageHandler
(
    void
)
{
    LE_INFO("Start Testmbx_AddRxMessageHandler");

    LE_ASSERT((HandlerRef = le_smsInbox1_AddRxMessageHandler(TestRxMsgHandler,
                                                            NULL)) != NULL);

    LE_INFO("End Testmbx_AddRxMessageHandler");
}

//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    LE_INFO("Exit and remove Rx message handler");
    le_smsInbox1_RemoveRxMessageHandler(HandlerRef);
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Helper.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage()
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
            "Usage of the smsInboxTest app is:",
            "   app runProc smsInboxTest --exe=smsInboxTest -- <read/receive>"};

    for(idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        if(sandboxed)
        {
            LE_INFO("%s", usagePtr[idx]);
        }
        else
        {
            fprintf(stderr, "%s\n", usagePtr[idx]);
        }
    }
}

// -------------------------------------------------------------------------------------------------
/**
 *  Test main function.
 */
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    if (le_arg_NumArgs() >= 1)
    {
        // Maximum number of messages configured for SMS Inbox.
        uint32_t maxMsgCount;

        LE_INFO("======== Start SMS Inbox service test ======== ");

        // Register a signal event handler for SIGINT when user interrupts/terminates process
        signal(SIGINT, SigHandler);

        const char* testCase = le_arg_GetArg(0);
        if (NULL == testCase)
        {
            LE_ERROR("testCase is NULL");
            exit(EXIT_FAILURE);
        }
        LE_INFO("   Test case.%s", testCase);

        MyMbx1Ref = le_smsInbox1_Open ();

        LE_ASSERT(LE_OVERFLOW == le_smsInbox1_SetMaxMessages(MAX_MESSAGE_INVALID_COUNT));
        LE_ASSERT_OK(le_smsInbox1_SetMaxMessages(MAX_MESSAGE_VALID_COUNT));

        LE_ASSERT(LE_BAD_PARAMETER == le_smsInbox1_GetMaxMessages(NULL));
        LE_ASSERT_OK(le_smsInbox1_GetMaxMessages(&maxMsgCount));
        LE_ASSERT(MAX_MESSAGE_VALID_COUNT == maxMsgCount);

        if (strncmp(testCase, "read", strlen("read")) == 0)
        {
            Testmbx_GetMessages();
            LE_INFO("======== SMS Inbox service test ended successfully ========");
            exit(EXIT_SUCCESS);
        }
        else if (strncmp(testCase, "receive", strlen("receive")) == 0)
        {
            Testmbx_AddRxMessageHandler();
            LE_INFO("======== SMS Inbox service test started successfully ========");
        }
        else
        {
            PrintUsage();
            LE_INFO("EXIT smsInboxTest");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT smsInboxTest");
        exit(EXIT_FAILURE);
    }
}

