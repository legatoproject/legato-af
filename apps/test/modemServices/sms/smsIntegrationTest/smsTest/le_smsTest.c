/**
  * This module implements the le_sms's integration tests.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"
#include <semaphore.h>


#define SHORT_TEXT_TEST_PATTERN  "Short"
#define LARGE_TEXT_TEST_PATTERN  "Large Text Test pattern Large Text Test pattern Large Text" \
     " Test pattern Large Text Test pattern Large Text Test pattern Large Text Test patt"
#define TEXT_TEST_PATTERN        "Text Test pattern"
#define FAIL_TEXT_TEST_PATTERN   "Fail Text Test pattern Fail Text Test pattern Fail Text Test" \
    " pattern Fail Text Test pattern Fail Text Test pattern Fail Text Test pattern Fail" \
    " Text Test pattern Text Test pattern "

#define NB_SMS_ASYNC_TO_SEND     3

#define PDU_MAX_LEN 100  // Maximum length of PDU message.

#define SMSC_LENGTH 0x00 // SMSC information stored in the phone has been used.
#define SMS_SUBMIT  0x11 // SMS-SUBMIT message.
#define TP_MSG_REF  0x00 // TP-Message-Reference.
#define PHN_FORMAT  0x81 // National format of the phone number.
#define TP_PID      0x00 // Protocol identifier.
#define TP_VLD_PER  0xAA // TP-Validity period set to 4 days.


typedef enum
{
    PDU_PATTERN_7BITS,               // 7-bit PDU message.
    PDU_PATTERN_8BITS,               // 8-bit PDU message.
    PDU_PATTERN_PTP_DCS_0x10_7BITS,  // TP-DCS is 0x10 - class 0 PDU type.
    PDU_PATTERN_PTP_DCS_0xC8_7BITS,  // TP-DCS is 0xC8 - turn on voice mailbox indicator.
    PDU_PATTERN_PTP_DCS_0xC0_7BITS   // TP-DCS is 0xC0 - turn off voice mailbox indicator.
} PduType_t;

/*
 * UCS2 test pattern
 */
static const uint16_t UCS2_TEST_PATTERN[] =
{   0x4900, 0x7400, 0x2000, 0x6900, 0x7300, 0x2000, 0x7400, 0x6800,
    0x6500, 0x2000, 0x5600, 0x6F00, 0x6900, 0x6300, 0x6500, 0x2000,
    0x2100, 0x2100, 0x2100, 0x2000, 0x4100, 0x7200, 0x6500, 0x2000,
    0x7900, 0x6f00, 0x7500, 0x2000, 0x7200, 0x6500, 0x6100, 0x6400,
    0x7900, 0x2000, 0x3F00
};

/*
 * Pdu message can be created with this link http://www.smartposition.nl/resources/sms_pdu.html
 */
// Sample PDU message in 7-bit format.
unsigned char SAMPLE_PDU_MSG_7BITS[] =
                   {0x0C, 0xC8, 0xF7, 0x1D, 0x14, 0x96, 0x97, 0x41, 0xF9, 0x77, 0xFD, 0x07};

// Sample PDU message in 8-bit format.
unsigned char SAMPLE_PDU_MSG_8BITS[] =
                   {0x0C, 0x48, 0x6F, 0x77, 0x20, 0x61, 0x72, 0x65, 0x20, 0x79, 0x6F, 0x75, 0x3F};

static uint8_t BINARY_TEST_PATTERN[] = {0x05,0x01,0x00,0x0A};

static le_sms_RxMessageHandlerRef_t RxHdlrRef = NULL;
static le_sms_FullStorageEventHandlerRef_t FullStorageHdlrRef = NULL;

static char DEST_TEST_PATTERN[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

static sem_t SmsRxSynchronization;
static sem_t SmsTxSynchronization;

static le_thread_Ref_t  RxThread = NULL;
static le_thread_Ref_t  TxCallBack = NULL;

static uint32_t NbSmsRx;
static uint32_t NbSmsTx;

static uint8_t pdu[PDU_MAX_LEN];
static int pduLength;

//--------------------------------------------------------------------------------------------------
//                                       Test Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Construct PDU message according to the PTP-DCS type.
*/
//--------------------------------------------------------------------------------------------------
static void BuildPdu
(
    PduType_t pduType,      ///< [IN]  PDU type.
    uint8_t*  pdu,          ///< [OUT] PDU string.
    int*      pduLength     ///< [OUT] PDU length.
)
{
    int i, j, value, decValue;
    int phoneNoLength = 0, pduIndex = 5;
    char phoneNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    uint8_t hexIndex[2] = {16, 1};

    pdu[0] = SMSC_LENGTH;
    pdu[1] = SMS_SUBMIT;
    pdu[2] = TP_MSG_REF;
    pdu[4] = PHN_FORMAT;

    phoneNoLength = strlen(DEST_TEST_PATTERN);

    for (i = 0; i < phoneNoLength; i += 2)
    {
        if ((phoneNoLength - i) == 1)
        {
            break;
        }

        // To swap the digits for PDU formation.
        phoneNumber[i] = DEST_TEST_PATTERN[i + 1];
        phoneNumber[i + 1] = DEST_TEST_PATTERN[i];

        decValue = 0;

        for (j = 0; j < 2; j++)
        {
            value = phoneNumber[i + j];        // Get the ascii value for the digit 'character'.
            value = value - 48;                // Convert the ascii value to numeric value.
            decValue += (value * hexIndex[j]); // Form the decimal number from the hexadecimal
                                               // value.
        }

        pdu[pduIndex++] = decValue;
    }

    // If the phone number is odd digits, then the last digit is to be combined
    // with 'F' to form pair and then swapped.
    if (phoneNoLength % 2 != 0)
    {
        pdu[3] = phoneNoLength + 1;

        phoneNumber[i] = 'F';
        phoneNumber[i + 1] = DEST_TEST_PATTERN[i];
        decValue = 240;
        value = phoneNumber[i + 1];
        value = value - 48;
        decValue += value;
        pdu[pduIndex++] = decValue;
    }
    else
    {
        pdu[3] = phoneNoLength;
    }

    pdu[pduIndex++] = TP_PID;

    int strLength = 0;
    switch(pduType)
    {
        case PDU_PATTERN_7BITS:
            pdu[pduIndex++] = 0x00;
            pdu[pduIndex++] = TP_VLD_PER;
            strLength = sizeof(SAMPLE_PDU_MSG_7BITS)/sizeof(SAMPLE_PDU_MSG_7BITS[0]);
            memcpy(&pdu[pduIndex], &SAMPLE_PDU_MSG_7BITS, strLength);
            break;

        case PDU_PATTERN_8BITS:
            pdu[pduIndex++] = 0x04;
            pdu[pduIndex++] = TP_VLD_PER;
            strLength = sizeof(SAMPLE_PDU_MSG_8BITS)/sizeof(SAMPLE_PDU_MSG_8BITS[0]);
            memcpy(&pdu[pduIndex], &SAMPLE_PDU_MSG_8BITS, strLength);
            break;

        case PDU_PATTERN_PTP_DCS_0x10_7BITS:
            pdu[pduIndex++] = 0x10;
            pdu[pduIndex++] = TP_VLD_PER;
            strLength = sizeof(SAMPLE_PDU_MSG_7BITS)/sizeof(SAMPLE_PDU_MSG_7BITS[0]);
            memcpy(&pdu[pduIndex], &SAMPLE_PDU_MSG_7BITS, strLength);
            break;

        case PDU_PATTERN_PTP_DCS_0xC8_7BITS:
            pdu[pduIndex++] = 0xC8;
            pdu[pduIndex++] = TP_VLD_PER;
            strLength = sizeof(SAMPLE_PDU_MSG_7BITS)/sizeof(SAMPLE_PDU_MSG_7BITS[0]);
            memcpy(&pdu[pduIndex], &SAMPLE_PDU_MSG_7BITS, strLength);
            break;

        case PDU_PATTERN_PTP_DCS_0xC0_7BITS:
            pdu[pduIndex++] = 0xC0;
            pdu[pduIndex++] = TP_VLD_PER;
            strLength = sizeof(SAMPLE_PDU_MSG_7BITS)/sizeof(SAMPLE_PDU_MSG_7BITS[0]);
            memcpy(&pdu[pduIndex], &SAMPLE_PDU_MSG_7BITS, strLength);
            break;

        default:
            break;
    }

    *pduLength = pduIndex + strLength;

    LE_DUMP(pdu, *pduLength);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set wait time for semaphore.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WaitFunction
(
    sem_t * semaphore,
    uint32_t delay
)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        LE_WARN("Cannot get current time");
        return LE_FAULT;
    }

    ts.tv_sec += delay;

    // Make the API synchronous
    if ( sem_timedwait(semaphore, &ts) == -1 )
    {
        if ( errno == EINTR)
        {
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS storage full message indication.
 *
 */
//--------------------------------------------------------------------------------------------------
static void StorageFullTestHandler
(
    le_sms_Storage_t  storage,
    void*            contextPtr
)
{
    LE_INFO("A Full storage SMS message is received. Type of full storage %d", storage);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS message reception.
 */
//--------------------------------------------------------------------------------------------------
static void TestRxHandler(le_sms_MsgRef_t msg, void* contextPtr)
{
    le_sms_Format_t       myformat;
    le_sms_Status_t       mystatus;
    le_result_t           res;
    char                  tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    char                  text[LE_SMS_TEXT_MAX_BYTES] = {0};
    uint8_t               smsPduDataPtr[LE_SMS_PDU_MAX_BYTES] = {0};
    uint16_t              smsUcs2Ptr[LE_SMS_UCS2_MAX_CHARS] = {0};
    size_t                smsUcs2Len;
    char                  smsPduDataHexString[(LE_SMS_PDU_MAX_BYTES * 2) + 1] = {0};
    size_t                uintval;
    size_t                smsPduLen1, smsPduLen2;

    LE_INFO("-TEST- New SMS message received ! msg.%p", msg);

    myformat = le_sms_GetFormat(msg);

    // Define the smsPduDataPtr buffer size
    smsPduLen1 = LE_SMS_PDU_MAX_BYTES;
    res = le_sms_GetPDU(msg, smsPduDataPtr, &smsPduLen1);

    if (res != LE_OK)
    {
        LE_ERROR("step FAILED !!");
        return;
    }

    smsPduLen2 = le_sms_GetPDULen(msg);
    // Check consistent between both APIs.

    LE_ASSERT(smsPduLen1 == smsPduLen2);

    if ( le_hex_BinaryToString(smsPduDataPtr, smsPduLen2, smsPduDataHexString,
            sizeof(smsPduDataHexString)) < 1 )
    {
        LE_ERROR("Failed to convert in hex string format!");
    }
    else
    {
        LE_INFO("Dump of PDU message (%d bytes): \"%s\"", (int) smsPduLen2, smsPduDataHexString);
    }

    if (myformat == LE_SMS_FORMAT_TEXT)
    {
        LE_INFO("SMS TEXT received");
        res = le_sms_GetSenderTel(msg, tel, 1);
        if(res != LE_OVERFLOW)
        {
            LE_ERROR("-TEST 1- Check le_sms_GetSenderTel failure (LE_OVERFLOW expected) !");
            LE_ASSERT(res != LE_OVERFLOW);
        }
        else
        {
            LE_INFO("-TEST 1- Check le_sms_GetSenderTel passed (LE_OVERFLOW expected).");
        }

        res = le_sms_GetSenderTel(msg, tel, sizeof(tel));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST 2- Check le_sms_GetSenderTel failure (LE_OK expected) !");
            LE_ASSERT(res != LE_OK);
        }
        else
        {
            LE_INFO("-TEST 2- Check le_sms_GetSenderTel passed (%s) (LE_OK expected).", tel);
        }

        if(strncmp(&tel[strlen(tel)-4], &DEST_TEST_PATTERN[strlen(DEST_TEST_PATTERN)-4], 4))
        {
            LE_ERROR("-TEST  3- Check le_sms_GetSenderTel, bad Sender Telephone number! (%s)",
                            tel);
            LE_ASSERT(true);
        }
        else
        {
            LE_INFO("-TEST  3- Check le_sms_GetSenderTel, Sender Telephone number OK.");
        }

        uintval = le_sms_GetUserdataLen(msg);
        if((uintval != strlen(TEXT_TEST_PATTERN)) &&
           (uintval != strlen(SHORT_TEXT_TEST_PATTERN)) &&
           (uintval != strlen(LARGE_TEXT_TEST_PATTERN)))
        {
            LE_ERROR("-TEST  4- Check le_sms_GetLen, bad expected text length! (%zd)", uintval);
            LE_ASSERT(true);
        }
        else
        {
            LE_INFO("-TEST  4- Check le_sms_GetLen OK.");
        }

        res = le_sms_GetTimeStamp(msg, timestamp, 1);
        if(res != LE_OVERFLOW)
        {
            LE_ERROR("-TEST  5- Check le_sms_GetTimeStamp -LE_OVERFLOW error- failure!");
            LE_ASSERT(res != LE_OVERFLOW);
        }
        else
        {
            LE_INFO("-TEST  5- Check le_sms_GetTimeStamp -LE_OVERFLOW error- OK.");
        }

        res = le_sms_GetTimeStamp(msg, timestamp, sizeof(timestamp));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  6- Check le_sms_GetTimeStamp failure!");
            LE_ASSERT(res != LE_OK);
        }
        else
        {
            LE_INFO("-TEST  6- Check le_sms_GetTimeStamp OK (%s).", timestamp);
        }

        res = le_sms_GetText(msg, text, sizeof(text));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  7- Check le_sms_GetText failure!");
            LE_ASSERT(res != LE_OK);
        }
        else
        {
            LE_INFO("-TEST  7- Check le_sms_GetText OK.");
        }

        if((strncmp(text, TEXT_TEST_PATTERN, strlen(TEXT_TEST_PATTERN)) != 0) &&
           (strncmp(text, SHORT_TEXT_TEST_PATTERN, strlen(SHORT_TEXT_TEST_PATTERN)) != 0) &&
           (strncmp(text, LARGE_TEXT_TEST_PATTERN, strlen(LARGE_TEXT_TEST_PATTERN)) != 0)
        )
        {
            LE_ERROR("-TEST  8- Check le_sms_GetText, bad expected received text! (%s)", text);
            LE_ASSERT(true);
        }
        else
        {
            LE_INFO("-TEST  8- Check le_sms_GetText, received text OK.");
        }

        // Verify that the message is read-only
        res = le_sms_SetDestination(msg, DEST_TEST_PATTERN);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  9- Check le_sms_SetDestination, parameter check failure!");
            LE_ASSERT(res != LE_NOT_PERMITTED);
        }
        else
        {
            LE_INFO("-TEST  9- Check le_sms_SetDestination OK.");
        }

        res = le_sms_SetText(msg, TEXT_TEST_PATTERN);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  10- Check le_sms_SetText, parameter check failure!");
            LE_ASSERT(res != LE_NOT_PERMITTED);
        }
        else
        {
            LE_INFO("-TEST  10- Check le_sms_SetText OK.");
        }

        // Verify Mark Read/Unread functions
        le_sms_MarkRead(msg);

        mystatus = le_sms_GetStatus(msg);
        if(mystatus != LE_SMS_RX_READ)
        {
            LE_ERROR("-TEST  11- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            LE_ASSERT(mystatus != LE_SMS_RX_READ);
        }
        else
        {
            LE_INFO("-TEST  11- Check le_sms_GetStatus, status OK.");
        }

        le_sms_MarkUnread(msg);

        mystatus = le_sms_GetStatus(msg);
        if(mystatus != LE_SMS_RX_UNREAD)
        {
            LE_ERROR("-TEST  12- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            LE_ASSERT(mystatus != LE_SMS_RX_UNREAD);
        }
        else
        {
            LE_INFO("-TEST  12- Check le_sms_GetStatus, status OK.");
        }

        res = le_sms_DeleteFromStorage(msg);
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  13- Check le_sms_DeleteFromStorage failure!");
            LE_ASSERT(res != LE_OK);
        }
        else
        {
            LE_INFO("-TEST  13- Check le_sms_DeleteFromStorage OK.");
        }
        NbSmsRx --;
    }
    if (myformat == LE_SMS_FORMAT_UCS2)
    {
        LE_INFO("SMS UCS2 received");
        res = le_sms_GetSenderTel(msg, tel, 1);
        if(res != LE_OVERFLOW)
        {
            LE_ERROR("-TEST 1- Check le_sms_GetSenderTel failure (LE_OVERFLOW expected) !");
            LE_ASSERT(res != LE_OVERFLOW);
        }
        else
        {
            LE_INFO("-TEST 1- Check le_sms_GetSenderTel passed (LE_OVERFLOW expected).");
        }

        res = le_sms_GetSenderTel(msg, tel, sizeof(tel));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST 2- Check le_sms_GetSenderTel failure (LE_OK expected) !");
            LE_ASSERT(res != LE_OK);
        }
        else
        {
            LE_INFO("-TEST 2- Check le_sms_GetSenderTel passed (%s) (LE_OK expected).", tel);
        }

        if(strncmp(&tel[strlen(tel)-4], &DEST_TEST_PATTERN[strlen(DEST_TEST_PATTERN)-4], 4))
        {
            LE_ERROR("-TEST  3- Check le_sms_GetSenderTel, bad Sender Telephone number! (%s)",
                tel);
            LE_ASSERT(true);
        }
        else
        {
            LE_INFO("-TEST  3- Check le_sms_GetSenderTel, Sender Telephone number OK.");
        }

        uintval = le_sms_GetUserdataLen(msg);
        if(uintval != (sizeof(UCS2_TEST_PATTERN) /2))
        {
            LE_ERROR("-TEST  4- Check le_sms_GetLen, bad expected text length! (%zd)", uintval);
            LE_ASSERT(uintval != (sizeof(UCS2_TEST_PATTERN) /2));
        }
        else
        {
            LE_INFO("-TEST  4- Check le_sms_GetLen OK.");
        }

        res = le_sms_GetTimeStamp(msg, timestamp, 1);
        if(res != LE_OVERFLOW)
        {
            LE_ERROR("-TEST  5- Check le_sms_GetTimeStamp -LE_OVERFLOW error- failure!");
            LE_ASSERT(res != LE_OVERFLOW);
        }
        else
        {
            LE_INFO("-TEST  5- Check le_sms_GetTimeStamp -LE_OVERFLOW error- OK.");
        }

        res = le_sms_GetTimeStamp(msg, timestamp, sizeof(timestamp));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  6- Check le_sms_GetTimeStamp failure!");
            LE_ASSERT(res != LE_OK);
        }
        else
        {
            LE_INFO("-TEST  6- Check le_sms_GetTimeStamp OK (%s).", timestamp);
        }
        smsUcs2Len = sizeof(smsUcs2Ptr) / 2;
        res = le_sms_GetUCS2(msg, smsUcs2Ptr, &smsUcs2Len);
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  7- Check le_sms_GetUcs2 failure!");
            LE_ASSERT(res != LE_OK);
        }
        else
        {
            LE_INFO("-TEST  7- Check le_sms_GetUcs2 OK.");
        }

        if(memcmp(smsUcs2Ptr, UCS2_TEST_PATTERN, sizeof(UCS2_TEST_PATTERN)) != 0)
        {
            LE_ERROR("-TEST  8- Check le_sms_GetUcs2, bad expected received UCS2!");
            LE_ASSERT(true);
        }
        else
        {
            LE_INFO("-TEST  8- Check le_sms_GetUcs2, received text OK.");
        }

        // Verify that the message is read-only
        res = le_sms_SetDestination(msg, DEST_TEST_PATTERN);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  9- Check le_sms_SetDestination, parameter check failure!");
            LE_ASSERT(res != LE_NOT_PERMITTED);
        }
        else
        {
            LE_INFO("-TEST  9- Check le_sms_SetDestination OK.");
        }

        res = le_sms_SetUCS2(msg, UCS2_TEST_PATTERN, sizeof(UCS2_TEST_PATTERN) / 2);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  10- Check le_sms_SetUCS2, parameter check failure!");
            LE_ASSERT(res != LE_NOT_PERMITTED);
        }
        else
        {
            LE_INFO("-TEST  10- Check UCS2_TEST_PATTERN OK.");
        }

        // Verify Mark Read/Unread functions
        le_sms_MarkRead(msg);

        mystatus = le_sms_GetStatus(msg);
        if(mystatus != LE_SMS_RX_READ)
        {
            LE_ERROR("-TEST  11- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            LE_ASSERT(mystatus != LE_SMS_RX_READ);
        }
        else
        {
            LE_INFO("-TEST  11- Check le_sms_GetStatus, status OK.");
        }

        le_sms_MarkUnread(msg);

        mystatus = le_sms_GetStatus(msg);
        if(mystatus != LE_SMS_RX_UNREAD)
        {
            LE_ERROR("-TEST  12- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            LE_ASSERT(mystatus != LE_SMS_RX_UNREAD);
        }
        else
        {
            LE_INFO("-TEST  12- Check le_sms_GetStatus, status OK.");
        }

        res = le_sms_DeleteFromStorage(msg);
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  13- Check le_sms_DeleteFromStorage failure!");
            LE_ASSERT(res != LE_OK);
        }
        else
        {
            LE_INFO("-TEST  13- Check le_sms_DeleteFromStorage OK.");
        }
        NbSmsRx --;
    }
    if (myformat == LE_SMS_FORMAT_UNKNOWN)
    {
        LE_INFO("SMS LE_SMS_FORMAT_UNKNOWN received");
        // Verify Mark Read/Unread functions
        le_sms_MarkRead(msg);

        mystatus = le_sms_GetStatus(msg);
        if(mystatus != LE_SMS_RX_READ)
        {
            LE_ERROR("-TEST  1- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            LE_ASSERT(mystatus != LE_SMS_RX_READ);
        }
        else
        {
            LE_INFO("-TEST  1- Check le_sms_GetStatus, status OK.");
        }

        le_sms_MarkUnread(msg);

        mystatus = le_sms_GetStatus(msg);
        if(mystatus != LE_SMS_RX_UNREAD)
        {
            LE_ERROR("-TEST  2- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            LE_ASSERT(mystatus != LE_SMS_RX_UNREAD);
        }
        else
        {
            LE_INFO("-TEST  2- Check le_sms_GetStatus, status OK.");
        }

        // Verify that the message is read-only
        res = le_sms_SetDestination(msg, DEST_TEST_PATTERN);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  3- Check le_sms_SetDestination, parameter check failure!");
            LE_ASSERT(res != LE_NOT_PERMITTED);
        }
        else
        {
            LE_INFO("-TEST  3- Check le_sms_SetDestination OK.");
        }

        res = le_sms_SetText(msg, TEXT_TEST_PATTERN);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  4- Check le_sms_SetText, parameter check failure!");
            LE_ASSERT(res != LE_NOT_PERMITTED);
        }
        else
        {
            LE_INFO("-TEST  4- Check le_sms_SetText OK.");
        }

        res = le_sms_SetUCS2(msg, UCS2_TEST_PATTERN, sizeof(UCS2_TEST_PATTERN) / 2);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  5- Check le_sms_SetUCS2, parameter check failure!");
            LE_ASSERT(res != LE_NOT_PERMITTED);
        }
        else
        {
            LE_INFO("-TEST  5- Check le_sms_SetUCS2 OK.");
        }

        res = le_sms_DeleteFromStorage(msg);
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  6- Check le_sms_DeleteFromStorage failure!");
            LE_ASSERT(res != LE_OK);
        }
        else
        {
            LE_INFO("-TEST  6- Check le_sms_DeleteFromStorage OK.");
        }
        NbSmsRx --;
    }
    else
    {
        LE_WARN("-TEST- I check only Text message!");
    }

    le_sms_Delete(msg);

    LE_INFO("Number of SMS remaining %d", NbSmsRx);
    if (NbSmsRx == 0)
    {
        sem_post(&SmsRxSynchronization);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Rx thread
 */
//--------------------------------------------------------------------------------------------------
static void* MyRxThread
(
    void* context   ///< See parameter documentation above.
)
{
    le_sms_ConnectService();

    RxHdlrRef = le_sms_AddRxMessageHandler(TestRxHandler, context);
    FullStorageHdlrRef = le_sms_AddFullStorageEventHandler(StorageFullTestHandler, context);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * SMS Callback
 */
//--------------------------------------------------------------------------------------------------
static void CallbackTestHandler
(
    le_sms_MsgRef_t msgRef,
    le_sms_Status_t status,
    void* contextPtr
)
{
    LE_INFO("Message %p, status %d, ctx %p", msgRef, status, contextPtr);
    le_sms_Delete(msgRef);

    if (!contextPtr)
    {
        LE_ERROR_IF(status != LE_SMS_SENT, "Test FAILED");
    }
    else
    {
        LE_DEBUG("Message sent successfully.");
    }

    NbSmsTx--;
    LE_INFO("Number of callback event remaining %d", NbSmsTx);

    if (NbSmsTx == 0)
    {
        sem_post(&SmsTxSynchronization);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * TX thread
 */
//--------------------------------------------------------------------------------------------------
static void* MyTxThread
(
    void* context   ///< See parameter documentation above.
)
{
    le_sms_ConnectService();
    bool pdu_type = *((bool *) context);

    int i;
    le_sms_MsgRef_t myMsg;

    for (i = NB_SMS_ASYNC_TO_SEND; i > 0; i--)
    {
        if (pdu_type)
        {
            BuildPdu(PDU_PATTERN_7BITS, pdu, &pduLength);

            myMsg = le_sms_SendPdu(pdu, pduLength, CallbackTestHandler, (void*) 1);
        }
        else
        {
            myMsg = le_sms_SendText(DEST_TEST_PATTERN, TEXT_TEST_PATTERN,
                            CallbackTestHandler, NULL);
        }
        LE_INFO("-TEST- Create sync text Msg %p", myMsg);

        myMsg = le_sms_Create();

        if (pdu_type)
        {
            BuildPdu(PDU_PATTERN_7BITS, pdu, &pduLength);

            le_sms_SetPDU(myMsg, pdu, pduLength);

            le_sms_SendAsync(myMsg, CallbackTestHandler,  (void*) 1);
        }
        else
        {
            le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
            le_sms_SetText(myMsg, TEXT_TEST_PATTERN);
            le_sms_SendAsync(myMsg, CallbackTestHandler, NULL);
        }

        LE_INFO("-TEST- Create Async text Msg %p", myMsg);
    }

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Send a Text message.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_sms_Send_Text
(
    void
)
{
    le_result_t           res = LE_FAULT;
    le_sms_MsgRef_t       myMsg;

    NbSmsRx = 2;

    // Init the semaphore for synchronous API (hangup, answer)
    sem_init(&SmsRxSynchronization,0,0);

    RxThread = le_thread_Create("Rx SMS reception", MyRxThread, NULL);
    le_thread_Start(RxThread);

    // Wait for thread starting.
    sleep(2);

    // Check if Thread SMS RX handler has been started
    if (!RxHdlrRef)
    {
        LE_ERROR("Handler SMS RX not ready !!");
        return LE_FAULT;
    }

    // Check if Thread SMS Storage indication handler has been started
    if (!FullStorageHdlrRef)
    {
        LE_ERROR("Handler SMS full storage not ready !!");
        return LE_FAULT;
    }

    myMsg = le_sms_Create();
    if (myMsg)
    {

        LE_DEBUG("-TEST- Create Msg %p", myMsg);

        res = le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_SetText(myMsg, LARGE_TEXT_TEST_PATTERN);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_Send(myMsg);
        if ((res == LE_FAULT) || (res == LE_FORMAT_ERROR))
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_SetText(myMsg, SHORT_TEXT_TEST_PATTERN);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_Send(myMsg);
        if ((res == LE_FAULT) || (res == LE_FORMAT_ERROR))
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = WaitFunction(&SmsRxSynchronization, 40000);
        LE_ERROR_IF(res != LE_OK, "SYNC FAILED");

        le_sms_Delete(myMsg);
    }

    le_sms_RemoveRxMessageHandler(RxHdlrRef);
    le_sms_RemoveFullStorageEventHandler(FullStorageHdlrRef);
    le_thread_Cancel(RxThread);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Send a UCS2 message.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_sms_Send_UCS2
(
    void
)
{
    le_result_t           res = LE_FAULT;
    le_sms_MsgRef_t       myMsg;

    NbSmsRx = 1;

    // Init the semaphore for synchronous API (hangup, answer)
    sem_init(&SmsRxSynchronization,0,0);

    RxThread = le_thread_Create("Rx SMS reception", MyRxThread, NULL);
    le_thread_Start(RxThread);

    // Wait for thread starting.
    sleep(2);

    // Check if Thread SMS RX handler has been started
    if (!RxHdlrRef)
    {
        LE_ERROR("Handler not ready !!");
        return LE_FAULT;
    }
    // Check if Thread SMS full Storage handler has been started
    if (!FullStorageHdlrRef)
    {
        LE_ERROR("Handler SMS full Storage not ready !!");
        return LE_FAULT;
    }

    myMsg = le_sms_Create();
    if (myMsg)
    {
        LE_DEBUG("-TEST- Create Msg %p", myMsg);

        res = le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_SetUCS2(myMsg, UCS2_TEST_PATTERN, sizeof(UCS2_TEST_PATTERN) / 2);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_Send(myMsg);
        if ((res == LE_FAULT) || (res == LE_FORMAT_ERROR))
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = WaitFunction(&SmsRxSynchronization, 120000);
        LE_ERROR_IF(res != LE_OK, "SYNC FAILED");

        le_sms_Delete(myMsg);
    }

    le_sms_RemoveRxMessageHandler(RxHdlrRef);
    le_sms_RemoveFullStorageEventHandler(FullStorageHdlrRef);
    le_thread_Cancel(RxThread);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Send a simple Text message with le_sms_SendText API().
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_sms_AsyncSendText
(
    void
)
{
    le_result_t res;
    bool pdu_type = false;

    NbSmsTx = NB_SMS_ASYNC_TO_SEND * 2 ;

    // Init the semaphore for asynchronous callback
    sem_init(&SmsTxSynchronization,0,0);

    TxCallBack = le_thread_Create("Tx CallBack", MyTxThread, &pdu_type);
    le_thread_Start(TxCallBack);

    res = WaitFunction(&SmsTxSynchronization, 120000);
    LE_ERROR_IF(res != LE_OK, "SYNC FAILED");

    le_thread_Cancel(TxCallBack);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Send a raw binary message.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_sms_Send_Binary
(
    void
)
{
    le_result_t           res = LE_FAULT;
    le_sms_MsgRef_t       myMsg;

    myMsg = le_sms_Create();
    if (myMsg)
    {
        LE_DEBUG("-TEST- Create Msg %p", myMsg);

        res = le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
        if (res != LE_OK)
        {
            return LE_FAULT;
        }

        res = le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(BINARY_TEST_PATTERN)/
                        sizeof(BINARY_TEST_PATTERN[0]));
        if (res != LE_OK)
        {
            return LE_FAULT;
        }

        res = le_sms_Send(myMsg);
        if ((res == LE_FAULT) || (res == LE_FORMAT_ERROR))
        {
            return LE_FAULT;
        }

        le_sms_Delete(myMsg);
        res = LE_OK;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread for testing MWI messages
 *
 */
//--------------------------------------------------------------------------------------------------
static void* MyTxThreadMwi
(
    void* context   ///< See parameter documentation above.
)
{
    le_sms_ConnectService();
    le_sms_MsgRef_t myMsg;

    BuildPdu(PDU_PATTERN_PTP_DCS_0xC0_7BITS, pdu, &pduLength);
    le_sms_SendPdu(pdu, pduLength, CallbackTestHandler, (void*) 2);

    myMsg = le_sms_Create();
    BuildPdu(PDU_PATTERN_PTP_DCS_0x10_7BITS, pdu, &pduLength);
    le_sms_SetPDU(myMsg, pdu, pduLength);
    le_sms_SendAsync(myMsg, CallbackTestHandler,  (void*) 2);

    myMsg = le_sms_Create();
    BuildPdu(PDU_PATTERN_PTP_DCS_0xC8_7BITS, pdu, &pduLength);
    le_sms_SetPDU(myMsg, pdu, pduLength);
    le_sms_SendAsync(myMsg, CallbackTestHandler,  (void*) 2);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Send a simple Text message with le_sms_SendPdu API().
 * ASYNC_PDU_TEST_PATTERN_7BITS PDU to encode.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_sms_AsyncSendPdu
(
    void
)
{
    le_result_t res;
    bool pdu_type = true;

    NbSmsTx = NB_SMS_ASYNC_TO_SEND;

    // Init the semaphore for asynchronous callback
    sem_init(&SmsTxSynchronization, 0, 0);

    TxCallBack = le_thread_Create("Tx CallBack", MyTxThread, &pdu_type);
    le_thread_Start(TxCallBack);

    res = WaitFunction(&SmsTxSynchronization, 10000);
    le_thread_Cancel(TxCallBack);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Send a Pdu message.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_sms_Send_Pdu
(
    void
)
{
    le_result_t          res = LE_FAULT;
    le_sms_MsgRef_t      myMsg;

    myMsg = le_sms_Create();
    if (myMsg)
    {
        LE_DEBUG("Create Msg %p", myMsg);

        BuildPdu(PDU_PATTERN_7BITS, pdu, &pduLength);

        res = le_sms_SetPDU(myMsg, pdu, pduLength);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_Send(myMsg);
        if ((res == LE_FAULT) || (res == LE_FORMAT_ERROR))
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        BuildPdu(PDU_PATTERN_8BITS, pdu, &pduLength);
        res = le_sms_SetPDU(myMsg, pdu, pduLength);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_Send(myMsg);
        if ((res == LE_FAULT) || (res == LE_FORMAT_ERROR))
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        le_sms_Delete(myMsg);
        res = LE_OK;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Send messages waiting indication (MWI)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_sms_Send_MwiSms
(
    void
)
{
    le_result_t res;
    bool pdu_type = true;
    NbSmsTx = 3;

    // Init the semaphore for asynchronous callback
    sem_init(&SmsTxSynchronization,0,0);

    TxCallBack = le_thread_Create("Tx CallBack", MyTxThreadMwi, &pdu_type);
    le_thread_Start(TxCallBack);

    res = WaitFunction(&SmsTxSynchronization, 10000);
    le_thread_Cancel(TxCallBack);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete ALL SMS receited
 */
//--------------------------------------------------------------------------------------------------
static void DeleteMessages
(
    void
)
{
    le_sms_MsgListRef_t     receivedList;
    le_sms_MsgRef_t         myMsg;

    receivedList = le_sms_CreateRxMsgList();
    if (receivedList)
    {
        myMsg = le_sms_GetFirst(receivedList);
        if (myMsg)
        {
            do
            {
                le_sms_DeleteFromStorage(myMsg);
                myMsg = le_sms_GetNext(receivedList);
            }
            while (myMsg != NULL);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Check Received List.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_sms_ReceivedList()
{
    le_result_t             res;
    le_sms_MsgRef_t         myMsg;
    le_sms_MsgRef_t         lMsg1, lMsg2;
    le_sms_MsgListRef_t     receivedList;
    le_sms_Status_t         mystatus;

    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);

    res = le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
    LE_ASSERT(res == LE_OK);

    res = le_sms_SetText(myMsg, TEXT_TEST_PATTERN);
    LE_ASSERT(res == LE_OK);

    res = le_sms_Send(myMsg);
    LE_ASSERT((res != LE_FAULT) && (res != LE_FORMAT_ERROR));

    res = le_sms_Send(myMsg);
    LE_ASSERT((res != LE_FAULT) && (res != LE_FORMAT_ERROR));

    sleep(10);

    // List Received messages
    receivedList = le_sms_CreateRxMsgList();
    if (receivedList)
    {
        lMsg1 = le_sms_GetFirst(receivedList);
        if (lMsg1 != NULL)
        {
            mystatus = le_sms_GetStatus(lMsg1);
            LE_ASSERT((mystatus == LE_SMS_RX_READ) || (mystatus == LE_SMS_RX_UNREAD));

            // Verify Mark Read functions on Rx message list
            le_sms_MarkRead(lMsg1);
            mystatus = le_sms_GetStatus(lMsg1);
            LE_ASSERT(mystatus == LE_SMS_RX_READ);

            // Verify Mark Unread functions on Rx message list
            le_sms_MarkUnread(lMsg1);
            mystatus = le_sms_GetStatus(lMsg1);
            LE_ASSERT(mystatus == LE_SMS_RX_UNREAD);

            // Verify Mark Read functions on Rx message list
            le_sms_MarkRead(lMsg1);
            mystatus = le_sms_GetStatus(lMsg1);
            LE_ASSERT(mystatus == LE_SMS_RX_READ);

            LE_INFO("-TEST- Delete Rx message 1 from storage.%p", lMsg1);
            le_sms_DeleteFromStorage(lMsg1);
        }
        else
        {
            LE_ERROR("Test required at less 2 SMSs in the storage");
            return LE_FAULT;
        }

        lMsg2 = le_sms_GetNext(receivedList);
        if (lMsg2 != NULL)
        {
            mystatus = le_sms_GetStatus(lMsg2);
            LE_ASSERT((mystatus == LE_SMS_RX_READ) || (mystatus == LE_SMS_RX_UNREAD));

            LE_INFO("-TEST- Delete Rx message 2 from storage.%p", lMsg2);
            le_sms_DeleteFromStorage(lMsg2);
        }
        else
        {
            LE_ERROR("Test requiered at less 2 SMSs in the storage");
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        LE_INFO("-TEST- Delete the ReceivedList");
        le_sms_DeleteList(receivedList);
    }
    le_sms_Delete(myMsg);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: testing the SMS storage area.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_sms_Storage
(
    void
)
{
    le_sms_Storage_t storage;

    LE_ASSERT(le_sms_SetPreferredStorage(LE_SMS_STORAGE_MAX) == LE_FAULT);

    LE_ASSERT(le_sms_SetPreferredStorage(LE_SMS_STORAGE_NV) == LE_OK);

    LE_ASSERT(le_sms_GetPreferredStorage(&storage) == LE_OK);

    LE_ASSERT(storage == LE_SMS_STORAGE_NV);

    LE_ASSERT(Testle_sms_Send_Text() == LE_OK);
    // test that pa_sms_DelMsgFromMem() called in TestRxHandler is
    // from storage 1 (PA_SMS_STORAGE_NV)

    LE_ASSERT(le_sms_SetPreferredStorage(LE_SMS_STORAGE_SIM) == LE_OK);

    LE_ASSERT(le_sms_GetPreferredStorage(&storage) == LE_OK);

    LE_ASSERT(storage == LE_SMS_STORAGE_SIM);

    LE_ASSERT(Testle_sms_Send_Text() == LE_OK);

    // test that pa_sms_DelMsgFromMem() called in TestRxHandler is
    // from storage 2 (PA_SMS_STORAGE_SIM)

    LE_INFO("Testle_sms_SetStorage PASSED");

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/*
 * ME must be registered on Network with the SIM in ready state.
 * Test application delete all Rx SM
 * Check "logread -f | grep sms" log
 * Start app : app start smsTest
 * Execute app : app runProc smsTest --exe=smsTest -- <Phone number>
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    if (le_arg_NumArgs() == 1)
    {
        const char* phoneNumber = le_arg_GetArg(0);
        if (NULL == phoneNumber)
        {
            LE_ERROR("phoneNumber is NULL");
            exit(EXIT_FAILURE);
        }
        strncpy((char*)DEST_TEST_PATTERN, phoneNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
        LE_INFO("Phone number %s", DEST_TEST_PATTERN);

        // Delete all Rx SMS message
        DeleteMessages();

        LE_INFO("======== SMS send async text test ========");
        LE_ASSERT(Testle_sms_AsyncSendText() == LE_OK);

        LE_INFO("======== SMS send async PDU test ========");
        LE_ASSERT_OK(Testle_sms_AsyncSendPdu());

        LE_INFO("======== SMS send binary test ========");
        LE_ASSERT(Testle_sms_Send_Binary() == LE_OK);

        LE_INFO("======== SMS send text test ========");
        LE_ASSERT(Testle_sms_Send_Text() == LE_OK);

        LE_INFO("======== SMS send PDU test ========");
        LE_ASSERT_OK(Testle_sms_Send_Pdu());

        LE_INFO("======== SMS receive list test ========");
        LE_ASSERT(Testle_sms_ReceivedList() == LE_OK);

        LE_INFO("======== SMS send UCS2 test ========");
        LE_ASSERT(Testle_sms_Send_UCS2() == LE_OK);

        LE_INFO("======== SMS send MwiSms test ========");
        LE_ASSERT_OK(Testle_sms_Send_MwiSms());

        LE_INFO("======== SMS storage test ========");
        LE_ASSERT(Testle_sms_Storage() == LE_OK);

        LE_INFO("smsTest sequence PASSED");

        // Delete all Rx SMS message
        DeleteMessages();
    }
    else
    {
        LE_ERROR("PRINT USAGE => app runProc smsTest --exe=smsTest -- <SIM Phone Number>");
    }

    exit(EXIT_SUCCESS);
}
