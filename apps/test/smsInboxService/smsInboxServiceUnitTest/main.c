/**
 * This module implements the unit tests for smsInboxService API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "log.h"
#include "le_log.h"


//--------------------------------------------------------------------------------------------------
/**
 * SMSInbox directory path.
 */
//--------------------------------------------------------------------------------------------------
#define SIMU_MSG_PATH           " /tmp/smsInbox/msg/"
#define SIMU_CONF_PATH          " /tmp/smsInbox/cfg/"

//--------------------------------------------------------------------------------------------------
/**
 * SMSInbox directory path length.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_FILE_PATH_LEN         512
#define MAX_SIMU_PATH_LEN         20
#define MAX_CMD_ARG               5

//--------------------------------------------------------------------------------------------------
/**
 * Maximum message count for SMS Inbox.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_MESSAGE_INVALID_COUNT 101
#define MAX_MESSAGE_COUNT         50

//--------------------------------------------------------------------------------------------------
/**
 * Session Reference
 */
//--------------------------------------------------------------------------------------------------
static le_smsInbox1_SessionRef_t MyMbx1Ref = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Msg ID's for text,pdu and binary messages
 */
//--------------------------------------------------------------------------------------------------
uint32_t MyMsgId1, MyMsgId2, MyMsgId3;

//--------------------------------------------------------------------------------------------------
/**
 * Rx Message Handler Reference
 */
//--------------------------------------------------------------------------------------------------
static le_smsInbox1_RxMessageHandlerRef_t HandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Test: Open smsInbox.
 *
 * API tested:
 * - le_smsInbox_Open
 *
 * Return:
 * - Reference on the opened message box.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_Open
(
    void
)
{
    MyMbx1Ref = le_smsInbox1_Open();
    LE_INFO("SmsInbox Open msg reference pointer is [%p]", MyMbx1Ref);
    LE_ASSERT(MyMbx1Ref != NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Close smsInbox.
 *
 * API tested:
 * - le_smsInbox_Close
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_Close
(
    void
)
{
    le_smsInbox1_Close(MyMbx1Ref);
    LE_INFO("SmsInbox msg reference Closed");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Delete msg smsInbox.
 *
 * API tested:
 * - le_smsInbox_DeleteMsg
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_DeleteMsg
(
    void
)
{
    le_smsInbox1_DeleteMsg(MyMsgId3);
    LE_INFO("SmsInbox msg deleted");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GetFirst smsInbox.
 *
 * API tested:
 * - le_smsInbox_GetFirst
 *
 * Return:
 * - 0 No message found (message box parsing is over).
 * - Message identifier.
 * - LE_BAD_PARAMETER The message reference is invalid.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_GetFirst
(
    void
)
{
    MyMsgId1 = le_smsInbox1_GetFirst(MyMbx1Ref);
    LE_INFO("SmsInbox Get First msgId1 [%d]", MyMsgId1);
    LE_ASSERT(MyMsgId1 != 0);
    LE_ASSERT(le_smsInbox1_GetFirst(NULL) == LE_BAD_PARAMETER)
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GetNext smsInbox.
 *
 * API tested:
 * - le_smsInbox_GetNext
 *
 * Return:
 * - 0 No message found (message box parsing is over).
 * - Message identifier.
 * - LE_BAD_PARAMETER The message reference is invalid.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_GetNext
(
    void
)
{
    MyMsgId2 = le_smsInbox1_GetNext(MyMbx1Ref);
    MyMsgId3 = le_smsInbox1_GetNext(MyMbx1Ref);
    LE_ASSERT(MyMsgId2 != 0);
    LE_ASSERT(MyMsgId3 != 0);
    LE_INFO("SmsInbox Get Next msgId2 [%d], msgId3 [%d]", MyMsgId2, MyMsgId3);
    LE_ASSERT(le_smsInbox1_GetFirst(NULL) == LE_BAD_PARAMETER)
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Read/Unread status smsInbox.
 *
 * API tested:
 * - le_smsInbox_IsUnread
 *   Return:
 *   - True if the message is unread, false otherwise.
 *   - LE_BAD_PARAMETER Invalid reference provided.
 * - le_smsInbox_MarkRead
 * - le_smsInbox_MarkUnread
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_ReadUnreadStatus
(
    void
)
{
    LE_INFO("SmsInbox Message is marked as %s", (le_smsInbox1_IsUnread(MyMsgId1)) ? "Unread":"Read");
    le_smsInbox1_MarkUnread(MyMsgId1);
    LE_ASSERT(le_smsInbox1_IsUnread(MyMsgId1) == true);
    le_smsInbox1_MarkRead(MyMsgId1);
    LE_ASSERT(le_smsInbox1_IsUnread(MyMsgId1) == false);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GetImsi smsInbox.
 *
 * API tested:
 * - le_smsInbox_GetImsi
 *
 * Return:
 * - LE_OK            The function succeeded.
 * - LE_OVERFLOW      The imsiPtr buffer was too small for the IMSI.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_GetImsi
(
    void
)
{
    char imsi[LE_SIM_IMSI_BYTES];
    LE_ASSERT(le_smsInbox1_GetImsi(MyMsgId1, imsi, 1) == LE_OVERFLOW);
    LE_ASSERT_OK(le_smsInbox1_GetImsi(MyMsgId1, imsi, sizeof(imsi)));
    LE_INFO("SmsInbox IMSI tied to the message is \"%s\".", imsi);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GetTimeStamp smsInbox.
 *
 * API tested:
 * - le_smsInbox_GetTimeStamp
 *
 * Return:
 * - LE_OK            The function succeeded.
 * - LE_OVERFLOW      Timestamp number length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_GetTimeStamp
(
    void
)
{
    char timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    LE_ASSERT(le_smsInbox1_GetTimeStamp(MyMsgId2, timestamp, 1) == LE_OVERFLOW);
    LE_ASSERT_OK(le_smsInbox1_GetTimeStamp(MyMsgId2, timestamp, sizeof(timestamp)));
    LE_INFO("SmsInbox Timestamp of the message is \"%s\".", timestamp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GetBinary smsInbox.
 *
 * API tested:
 * - le_smsInbox_GetBinary
 *
 * Return:
 * - LE_OK            The function succeeded.
 * - LE_OVERFLOW      Message length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_GetBinary
(
    void
)
{
    uint8_t bin[LE_SMS_BINARY_MAX_BYTES];
    size_t uintval = 0;
    LE_ASSERT(le_smsInbox1_GetBinary(MyMsgId3, bin, &uintval) == LE_OVERFLOW);
    uintval = sizeof(bin);
    LE_ASSERT_OK(le_smsInbox1_GetBinary(MyMsgId3, bin, &uintval));
    LE_DUMP(bin, uintval);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GetText smsInbox.
 *
 * API tested:
 * - le_smsInbox_GetText
 *
 * Return:
 * - LE_OK            The function succeeded.
 * - LE_OVERFLOW      The imsiPtr buffer was too small for the IMSI.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_GetText
(
    void
)
{
    char text[LE_SMS_TEXT_MAX_BYTES];
    LE_ASSERT(le_smsInbox1_GetText(MyMsgId2, text, 1) == LE_OVERFLOW);
    LE_ASSERT_OK(le_smsInbox1_GetText(MyMsgId2, text, sizeof(text)));
    LE_INFO("SmsInbox Content of the TEXT message is \"%s\".", text);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Rx Message Handler for smsInbox.
 *
 * API tested:
 * - le_smsInbox_AddRxMessageHandler
 */
//--------------------------------------------------------------------------------------------------
static void TestRxMsgHandler
(
    uint32_t msgId,
    void*    contextPtr
)
{
    char imsi[LE_SIM_IMSI_BYTES];
    msgId = MyMsgId1;
    LE_ASSERT_OK(le_smsInbox1_GetImsi(msgId, imsi, sizeof(imsi)));
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: AddRxMessageHandler smsInbox.
 *
 * API tested:
 * - le_smsInbox_AddRxMessageHandler
 *
 * Return:
 * - Handler reference.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_AddRxMessageHandler
(
    void
)
{
    LE_ASSERT((HandlerRef = le_smsInbox1_AddRxMessageHandler(TestRxMsgHandler,
                                                            NULL)) != NULL);
    LE_INFO("SmsInbox Add RxMessageHandler %p", HandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: RemoveRxMessageHandler smsInbox.
 *
 * API tested:
 * - le_smsInbox_RemoveRxMessageHandler
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_RemoveRxMessageHandler
(
    void
)
{
    le_smsInbox1_RemoveRxMessageHandler(HandlerRef);
    LE_INFO("SmsInbox Remove RxMessageHandler");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GetFormat smsInbox.
 *
 * API tested:
 * - le_smsInbox_GetFormat
 *
 * Return:
 * - Message format.
 * - FORMAT_UNKNOWN when the message format cannot be identified or invalid.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_GetFormat
(
    void
)
{
    le_sms_Format_t format;
    format = le_smsInbox1_GetFormat(MyMsgId1);
    LE_INFO("SmsInbox msg1 format is %d.", format);
    LE_ASSERT(format == LE_SMS_FORMAT_PDU);
    format = le_smsInbox1_GetFormat(MyMsgId2);
    LE_INFO("SmsInbox msg2 format is %d.", format);
    LE_ASSERT(format == LE_SMS_FORMAT_TEXT);
    format = le_smsInbox1_GetFormat(MyMsgId3);
    LE_INFO("SmsInbox msg3 format is %d.", format);
    LE_ASSERT(format == LE_SMS_FORMAT_BINARY);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GetPdu smsInbox.
 *
 * API tested:
 * - le_smsInbox_GetPdu
 *
 * Return:
 * - LE_OK            The function succeeded.
 * - LE_OVERFLOW      Message length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_GetPdu
(
    void
)
{
    uint8_t pdu[LE_SMS_PDU_MAX_BYTES];
    size_t uintval = 1;
    LE_ASSERT(le_smsInbox1_GetPdu(MyMsgId1, pdu, &uintval) == LE_OVERFLOW);
    uintval = sizeof(pdu);
    LE_ASSERT_OK(le_smsInbox1_GetPdu(MyMsgId1, pdu, &uintval));
    LE_DUMP(pdu, uintval);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GetMsgLen smsInbox.
 *
 * API tested:
 * - le_smsInbox_GetMsgLen
 *
 * Return:
 * - Number of characters for text messages, or the length of the data.
 * - LE_BAD_PARAMETER The message reference is invalid.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_GetMsgLen
(
    void
)
{
    size_t uintval;
    uintval = le_smsInbox1_GetMsgLen(MyMsgId1);
    LE_INFO("Length of the message is %zd.", uintval);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GetSenderTel smsInbox.
 *
 * API tested:
 * - le_smsInbox_GetSenderTel
 *
 * Return:
 * - LE_OK            The function succeeded.
 * - LE_OVERFLOW      Identifier length exceed the maximum length.
 * - LE_BAD_PARAMETER The message reference is invalid.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_GetSenderTel
(
    void
)
{
    char tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    LE_ASSERT(le_smsInbox1_GetSenderTel(MyMsgId2, tel, 1) == LE_OVERFLOW);
    LE_ASSERT_OK(le_smsInbox1_GetSenderTel(MyMsgId2, tel, sizeof(tel)));
    LE_INFO("SmsInbox Sender telephone of the message is \"%s\".", tel);

}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate smsInbox config files
 */
//--------------------------------------------------------------------------------------------------
static void Simulate_smsInbox_cfgFileInit
(
   const char *smsCfgFilePath
)
{
    LE_INFO("Init Sms InBox cfg files");
    char cfgCpCommand[512] = "cp -rf ";
    size_t cfgFilePathLen = strlen(smsCfgFilePath);
    strncat(cfgCpCommand, smsCfgFilePath, cfgFilePathLen + 1);
    strncat(cfgCpCommand, SIMU_CONF_PATH, MAX_SIMU_PATH_LEN);
    system(cfgCpCommand);
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate smsInbox msg files
 */
//--------------------------------------------------------------------------------------------------
static void Simulate_smsInbox_msgFileInit
(
    const char *smsMsgFilePath
)
{
    LE_INFO("Init Sms InBox msg files");
    char msgCpCommand[512]= "cp -rf ";
    size_t msgFilePathLen = strlen(smsMsgFilePath);
    strncat(msgCpCommand, smsMsgFilePath, msgFilePathLen + 1);
    strncat(msgCpCommand, SIMU_MSG_PATH, MAX_SIMU_PATH_LEN);
    system(msgCpCommand);
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate smsInbox set Maximum messages supported for message box.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_SetMaxMessages
(
    uint32_t maxMessageCount
)
{
    LE_INFO("Set Maximum number of messages for message box");

    LE_ASSERT(LE_OVERFLOW == le_smsInbox1_SetMaxMessages(MAX_MESSAGE_INVALID_COUNT));
    LE_ASSERT_OK(le_smsInbox1_SetMaxMessages(maxMessageCount));
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate smsInbox get Maximum messages supported for message box.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_smsInbox_GetMaxMessages
(
    void
)
{
    uint32_t maxMessageCount;

    LE_INFO("Get Maximum number of messages for message box");

    LE_ASSERT(LE_BAD_PARAMETER == le_smsInbox1_GetMaxMessages(NULL));

    LE_ASSERT_OK(le_smsInbox1_GetMaxMessages(&maxMessageCount));
    LE_ASSERT(maxMessageCount == MAX_MESSAGE_COUNT);
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("======== START UnitTest of SMS INBOX API ========");
    const char* argString = "";

    if (le_arg_NumArgs() >= MAX_CMD_ARG)
    {
        argString = le_arg_GetArg(0);
        if (NULL == argString)
        {
            LE_ERROR("argString is NULL");
            exit(EXIT_FAILURE);
        }
        Simulate_smsInbox_cfgFileInit(argString);

        argString = le_arg_GetArg(1);
        if (NULL == argString)
        {
            LE_ERROR("argString is NULL");
            exit(EXIT_FAILURE);
        }
        Simulate_smsInbox_cfgFileInit(argString);

        argString = le_arg_GetArg(2);
        if (NULL == argString)
        {
            LE_ERROR("argString is NULL");
            exit(EXIT_FAILURE);
        }
        Simulate_smsInbox_msgFileInit(argString);

        argString = le_arg_GetArg(3);
        if (NULL == argString)
        {
            LE_ERROR("argString is NULL");
            exit(EXIT_FAILURE);
        }
        Simulate_smsInbox_msgFileInit(argString);

        argString = le_arg_GetArg(4);
        if (NULL == argString)
        {
            LE_ERROR("argString is NULL");
            exit(EXIT_FAILURE);
        }
        Simulate_smsInbox_msgFileInit(argString);

    }

    LE_INFO("======== smsInbox Open test ========");
    Testle_smsInbox_Open();

    LE_INFO("======== smsInbox SetMaxMessages test ========");
    Testle_smsInbox_SetMaxMessages(MAX_MESSAGE_COUNT);

    LE_INFO("======== smsInbox GetMaxMessages test ========");
    Testle_smsInbox_GetMaxMessages();

    LE_INFO("======== smsInbox GetFirst Msg test ========");
    Testle_smsInbox_GetFirst();

    LE_INFO("======== smsInbox GetNext test ========");
    Testle_smsInbox_GetNext();

    LE_INFO("======== smsInbox MarkRead test ========");
    Testle_smsInbox_ReadUnreadStatus();

    LE_INFO("======== smsInbox GetImsi test ========");
    Testle_smsInbox_GetImsi();

    LE_INFO("======== smsInbox GetMsgLen test ========");
    Testle_smsInbox_GetMsgLen();

    LE_INFO("======== smsInbox GetFormat test ========");
    Testle_smsInbox_GetFormat();

    LE_INFO("======== smsInbox GetPdu test ========");
    Testle_smsInbox_GetPdu();

    LE_INFO("======== smsInbox GetSenderTel test ========");
    Testle_smsInbox_GetSenderTel();

    LE_INFO("======== smsInbox GetBinary test ========");
    Testle_smsInbox_GetBinary();

    LE_INFO("======== smsInbox GetText test ========");
    Testle_smsInbox_GetText();

    LE_INFO("======== smsInbox GetTimeStamp test ========");
    Testle_smsInbox_GetTimeStamp();

    LE_INFO("======== smsInbox AddRxMessageHandler test ========");
    Testle_smsInbox_AddRxMessageHandler();

    LE_INFO("======== smsInbox RemoveRxMessageHandler test ========");
    Testle_smsInbox_RemoveRxMessageHandler();

    LE_INFO("======== smsInbox delete test ========");
    Testle_smsInbox_DeleteMsg();

    LE_INFO("======== smsInbox Close test ========");
    Testle_smsInbox_Close();

    LE_INFO("======== UnitTest of SMS INBOX  API FINISHED ========");
    exit(EXIT_SUCCESS);
}
