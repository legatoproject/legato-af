/**
  * This module implements the le_sms's unit tests.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"
#include "pa_sms.h"
#include "pa_sim.h"
#include "smsPdu.h"
#include "time.h"
#include "le_sms_local.h"
#include "pa_sms_simu.h"


//--------------------------------------------------------------------------------------------------
/**
 * Test sequence Structure list
 */
//--------------------------------------------------------------------------------------------------
#define LONG_TIMEOUT    5000
#define SHORT_TIMEOUT   1000
#define VOID_PATTERN  ""

#define SHORT_TEXT_TEST_PATTERN  "Short"
#define LARGE_TEXT_TEST_PATTERN  "Large Text Test pattern Large Text Test pattern Large Text" \
     " Test pattern Large Text Test pattern Large Text Test pattern Large Text Test patt"
#define TEXT_TEST_PATTERN        "Text Test pattern"

static uint8_t BINARY_TEST_PATTERN[]={0x05,0x01,0x00,0x0A};

static uint16_t UCS2_TEST_PATTERN[] =
    {   0x4900, 0x7400, 0x2000, 0x6900, 0x7300, 0x2000, 0x7400, 0x6800,
        0x6500, 0x2000, 0x5600, 0x6F00, 0x6900, 0x6300, 0x6500, 0x2000,
        0x2100, 0x2100, 0x2100, 0x2000, 0x4100, 0x7200, 0x6500, 0x2000,
        0x7900, 0x6f00, 0x7500, 0x2000, 0x7200, 0x6500, 0x6100, 0x6400,
        0x7900, 0x2000, 0x3F00
    };

static char DEST_TEST_PATTERN[] = "0123456789";

/*
 * Pdu message can be created with this link http://www.smartposition.nl/resources/sms_pdu.html
 */
static uint8_t PDU_TEST_PATTERN_7BITS[]=
                           {0x00,0x01,0x00,0x0A,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x11,0xD4,0x32,0x9E,0x0E,0xA2,0x96,0xE7,0x74,0x10,0x3C,0x4C,
                            0xA7,0x97,0xE5,0x6E
                           };
//Class 0
static uint8_t PDU_TEST_PATTERN_PTP_DCS_0x10_7BITS[]=
                           {0x00,0x50,0x92,0x46,0xF0,0x00,0x10,0x71,0x90,0x40,0x71,0x70,
                            0x90,0x80,0x05,0xE8,0x32,0x9B,0xFD,0x06
                           };
//Voice mailbox indicator
static uint8_t PDU_TEST_PATTERN_PTP_DCS_0xC8_7BITS[]=
                           {0x00,0x50,0x92,0x46,0xF0,0x00,0xC8,0x71,0x90,0x40,0x11,0x64,
                            0x50,0x80,0x00
                           };
//Message waiting indicator
static uint8_t PDU_TEST_PATTERN_PTP_DCS_0xC0_7BITS[]=
                           {0x00,0x50,0x92,0x46,0xF0,0x00,0xC0,0x71,0x90,0x40,0x11,0x93,
                            0x63,0x80,0x00
                           };

static uint8_t PDU_TEST_PATTERN_8BITS[]=
                           {0x00,0x01,0x00,0x0A,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x11,0x54,0x65,0x78,0x74,0x20,0x54,0x65,0x73,0x74,0x20,0x70,
                            0x61,0x74,0x74,0x65,0x72,0x6E
                           };

static uint8_t PDU_RECEIVE_TEST_PATTERN_7BITS[]=
            {0x07, 0x91, 0x33, 0x86, 0x09, 0x40, 0x00, 0xF0, 0x04, 0x0B,
            0x91, 0x33, 0x46, 0x53, 0x73, 0x19, 0xF9, 0x00, 0x00, 0x41,
            0x70, 0x13, 0x02, 0x55, 0x71, 0x80, 0x65, 0xCC, 0xB7, 0xBC,
            0xDC, 0x06, 0xA5, 0xE1, 0xF3, 0x7A, 0x1B, 0x44, 0x7E, 0xB3,
            0xDF, 0x72, 0xD0, 0x3C, 0x4D, 0x07, 0x85, 0xDB, 0x65, 0x3A,
            0x0B, 0x34, 0x7E, 0xBB, 0xE7, 0xE5, 0x31, 0xBD, 0x4C, 0xAF,
            0xCB, 0x41, 0x61, 0x72, 0x1A, 0x9E, 0x9E, 0x8F, 0xD3, 0xEE,
            0x33, 0xA8, 0xCC, 0x4E, 0xD3, 0x5D, 0xA0, 0xE6, 0x5B, 0x2E,
            0x4E, 0x83, 0xD2, 0x6E, 0xD0, 0xF8, 0xDD, 0x6E, 0xBF, 0xC9,
            0x6F, 0x10, 0xBB, 0x3C, 0xA6, 0xD7, 0xE7, 0x2C, 0x50, 0xBC,
            0x9E, 0x9E, 0x83, 0xEC, 0x6F, 0x76, 0x9D, 0x0E, 0x0F, 0xD3,
            0x41, 0x65, 0x79, 0x98, 0xEE, 0x02,
            };

static uint8_t PDU_RECEIVE_TEST_PATTERN_SMSPDU_UCS2_16_BITS[]=
            {0x07, 0x91, 0x33, 0x66, 0x00, 0x30, 0x00, 0xF0, 0x04, 0x0B,
            0x91, 0x33, 0x66, 0x92, 0x12, 0x37, 0xF0, 0x00, 0x08, 0x61,
            0x10, 0x12, 0x51, 0x10, 0x93, 0x40, 0x14, 0x00, 0x4D, 0x00,
            0x79, 0x00, 0x20, 0x00, 0x6D, 0x00, 0x65, 0x00, 0x73, 0x00,
            0x73, 0x00, 0x61, 0x00, 0x67, 0x00, 0x65,
            };


static uint8_t PDU_RECEIVE_TEST_PATTERN_BROADCAST_7BITS[]=
            {0x00, 0x01, 0x00, 0x90, 0x01, 0x11, 0xC5, 0x76, 0x59, 0x7E,
            0x2E, 0xBB, 0xC7, 0xF9, 0x50, 0x08, 0x44, 0x2D, 0xCF, 0xE9,
            0x20, 0xD0, 0xB0, 0x19, 0x9C, 0x82, 0x72, 0xB0
            };

static uint8_t PDU_RECEIVE_TEST_PATTERN_STATUS_REPORT[] =
{
    0x06, 0x0C, 0x0B, 0x91, 0x33, 0x21, 0x43, 0x65, 0x87, 0xF9,
    0x71, 0x90, 0x60, 0x90, 0x75, 0x00, 0x80, 0x71, 0x90, 0x60,
    0x90, 0x75, 0x00, 0x80, 0x00
};

//--------------------------------------------------------------------------------------------------
/**
 * Task context structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_thread_Ref_t appStorageFullThread;
    le_sms_FullStorageEventHandlerRef_t statHandler;
    le_sms_Storage_t storage;
} AppContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * SMS application structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_thread_Ref_t appSmsReceiveThread;
    le_sms_RxMessageHandlerRef_t rxHdlrRef;
} AppSmsContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * SMS counters structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t rx;     ///< Received messages counter
    int32_t tx;     ///< Sent messages counter
    int32_t rxCb;   ///< Received broadcast messages counter
} SmsCount_t;

//--------------------------------------------------------------------------------------------------
/**
 * Tests number enumeration
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SMS_SEND_TEST_NUMBER_1,
    SMS_SEND_TEST_NUMBER_2,
    SMS_SEND_TEST_NUMBER_3,
    SMS_SEND_TEST_NUMBER_4,
    SMS_SEND_TEST_NUMBER_5,
    SMS_SEND_TEST_NUMBER_6,
    SMS_SEND_TEST_NUMBER_7,
    SMS_SEND_TEST_NUMBER_8,
    SMS_SEND_TEST_NUMBER_9,
    SMS_SEND_TEST_NUMBER_10,
    SMS_SEND_TEST_NUMBER_TIMOUT,
    SMS_SEND_TEST_FAILED
}SmsSendTest_t;


//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Message objects.
 *
 */
//--------------------------------------------------------------------------------------------------

static AppContext_t AppCtx;
static le_sem_Ref_t SmsThreadSemaphore;
static AppSmsContext_t AppSmsReceiveCtx;
static le_sem_Ref_t SmsReceiveThreadSemaphore;
static le_sem_Ref_t SmsSendSemaphore;
static le_sms_MsgRef_t ReceivedSmsRef;

//--------------------------------------------------------------------------------------------------
/**
 * Test: Text Message Object Set/Get APIS.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_SetGetText
(
    void
)
{
    le_sms_MsgRef_t       myMsg;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    char                  tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    char                  text[LE_SMS_TEXT_MAX_BYTES] = {0};
    uint8_t               raw[LE_SMS_BINARY_MAX_BYTES];
    size_t                length;

    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);

    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);

    LE_ASSERT(le_sms_SetText(myMsg, VOID_PATTERN) == LE_BAD_PARAMETER);

    LE_ASSERT(le_sms_SetText(myMsg, TEXT_TEST_PATTERN) == LE_OK);

    LE_ASSERT(le_sms_GetFormat(myMsg) == LE_SMS_FORMAT_TEXT);

    LE_ASSERT(le_sms_GetSenderTel(myMsg, tel, sizeof(tel)) == LE_NOT_PERMITTED);

    LE_ASSERT(le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp)) == LE_NOT_PERMITTED);

    LE_ASSERT(le_sms_GetUserdataLen(myMsg) == strlen(TEXT_TEST_PATTERN));

    LE_ASSERT(le_sms_GetText(myMsg, text, 1) == LE_OVERFLOW);

    LE_ASSERT(le_sms_GetText(myMsg, text, sizeof(text)) == LE_OK);

    LE_ASSERT(strncmp(text, TEXT_TEST_PATTERN, strlen(TEXT_TEST_PATTERN)) == 0);

    LE_ASSERT(le_sms_SetDestination(myMsg, VOID_PATTERN) == LE_BAD_PARAMETER);

    length=1;
    LE_ASSERT(le_sms_GetBinary(myMsg, raw, &length) == LE_FORMAT_ERROR);

    le_sms_Delete(myMsg);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Raw binary Message Object Set/Get APIS.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_SetGetBinary
(
    void
)
{
    le_sms_MsgRef_t       myMsg;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    char                  tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    uint8_t               raw[LE_SMS_BINARY_MAX_BYTES];
    char                  text[LE_SMS_TEXT_MAX_BYTES] = {0};
    uint16_t              ucs2Raw[LE_SMS_UCS2_MAX_CHARS];
    size_t                length;
    uint32_t              i;

    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);

    LE_ASSERT(le_sms_SetDestination(myMsg, VOID_PATTERN) == LE_BAD_PARAMETER);
    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);

    LE_ASSERT(le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, (sizeof(PDU_TEST_PATTERN_7BITS)/
                            sizeof(PDU_TEST_PATTERN_7BITS[0])) ) == LE_OK);

    LE_ASSERT(le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, 0) == LE_BAD_PARAMETER);
    LE_ASSERT(le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(BINARY_TEST_PATTERN)) == LE_OK);

    LE_ASSERT(le_sms_GetFormat(myMsg) == LE_SMS_FORMAT_BINARY);

    LE_ASSERT(le_sms_GetSenderTel(myMsg, tel, sizeof(tel)) == LE_NOT_PERMITTED);

    LE_ASSERT(le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp)) == LE_NOT_PERMITTED);

    length = le_sms_GetUserdataLen(myMsg);
    LE_ASSERT(length == sizeof(BINARY_TEST_PATTERN));

    length=1;
    LE_ASSERT(le_sms_GetBinary(myMsg, raw, &length) == LE_OVERFLOW);

    length = sizeof(BINARY_TEST_PATTERN);
    LE_ASSERT(le_sms_GetBinary(myMsg, raw, &length) == LE_OK);

    for(i=0; i<sizeof(BINARY_TEST_PATTERN) ; i++)
    {
        LE_ASSERT(raw[i] == BINARY_TEST_PATTERN[i]);
    }

    LE_ASSERT(length == sizeof(BINARY_TEST_PATTERN));
    LE_ASSERT(le_sms_GetText(myMsg, text, 1) == LE_FORMAT_ERROR);

    length = 1;
    LE_ASSERT(le_sms_GetUCS2(myMsg, ucs2Raw, &length) == LE_FORMAT_ERROR);

    le_sms_Delete(myMsg);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: UCS2 content Message Object Set/Get APIS.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_SetGetUCS2
(
    void
)
{
    le_sms_MsgRef_t       myMsg;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    char                  tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    uint16_t              ucs2Raw[LE_SMS_UCS2_MAX_CHARS];
    size_t                length;
    uint32_t              i;

    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);

    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);

    LE_ASSERT(le_sms_SetUCS2(myMsg, UCS2_TEST_PATTERN, 0) == LE_BAD_PARAMETER);
    LE_ASSERT(le_sms_SetUCS2(myMsg, UCS2_TEST_PATTERN, sizeof(UCS2_TEST_PATTERN) / 2) == LE_OK);

    LE_ASSERT(le_sms_GetFormat(myMsg) == LE_SMS_FORMAT_UCS2);

    LE_ASSERT(le_sms_GetSenderTel(myMsg, tel, sizeof(tel)) == LE_NOT_PERMITTED);

    LE_ASSERT(le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp)) == LE_NOT_PERMITTED);

    length = le_sms_GetUserdataLen(myMsg);
    LE_ASSERT(length == (sizeof(UCS2_TEST_PATTERN) / 2));

    length = 1;
    LE_ASSERT(le_sms_GetUCS2(myMsg, ucs2Raw, &length) == LE_OVERFLOW);

    length = sizeof(UCS2_TEST_PATTERN);
    LE_ASSERT(le_sms_GetUCS2(myMsg, ucs2Raw, &length) == LE_OK);

    for(i=0; i < (sizeof(UCS2_TEST_PATTERN)/2); i++)
    {
        LE_ERROR_IF(ucs2Raw[i] != UCS2_TEST_PATTERN[i],
                    "%d/%d 0x%04X==0x%04X", (int) i, (int) sizeof(UCS2_TEST_PATTERN)/2,
                    ucs2Raw[i],
                    UCS2_TEST_PATTERN[i]);
        LE_ASSERT(ucs2Raw[i] == UCS2_TEST_PATTERN[i]);
    }

    LE_ASSERT(length == sizeof(UCS2_TEST_PATTERN) / 2);

    LE_ASSERT(le_sms_SetDestination(myMsg, VOID_PATTERN) == LE_BAD_PARAMETER);


    le_sms_Delete(myMsg);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: PDU Message Object Set/Get APIS.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_SetGetPDU
(
    void
)
{
    le_sms_MsgRef_t       myMsg;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    char                  tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    uint8_t               pdu[LE_SMS_PDU_MAX_BYTES];
    size_t                length;
    uint32_t              i;

    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);

    LE_ASSERT(le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, 0) == LE_BAD_PARAMETER);
    LE_ASSERT(le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, (sizeof(PDU_TEST_PATTERN_7BITS)/
                            sizeof(PDU_TEST_PATTERN_7BITS[0]))) == LE_OK);

    LE_ASSERT(le_sms_GetSenderTel(myMsg, tel, sizeof(tel)) == LE_NOT_PERMITTED);
    LE_ASSERT(le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp)) == LE_NOT_PERMITTED);

    length = le_sms_GetPDULen(myMsg);
    LE_ASSERT(length == (sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0])));

    length = 1;
    LE_ASSERT(le_sms_GetPDU(myMsg, pdu, &length) == LE_OVERFLOW);

    length = sizeof(pdu);
    LE_ASSERT(le_sms_GetPDU(myMsg, pdu, &length) == LE_OK);

    for(i=0; i<sizeof(PDU_TEST_PATTERN_7BITS) ; i++)
    {
        LE_ASSERT(pdu[i] == PDU_TEST_PATTERN_7BITS[i]);
    }

    LE_ASSERT(length == sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]));

    le_sms_Delete(myMsg);
}

//--------------------------------------------------------------------------------------------------
/*
 * Test case le_sms_GetSmsCenterAddress() and le_sms_SetSmsCenterAddress() APIs
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_SetGetSmsCenterAddress
(
    void
)
{
    char smscMdmRefStr[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};
    char smscMdmStr[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};
    char smscStrs[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = "+33123456789";

    // Get current SMS service center address.
    // Check LE_OVERFLOW error case
    LE_ASSERT(le_sms_GetSmsCenterAddress(smscMdmRefStr, 5) == LE_OVERFLOW);

    // Get current SMS service center address.
    LE_ASSERT(le_sms_GetSmsCenterAddress(smscMdmRefStr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == LE_OK);

    // Set "+33123456789" SMS service center address.
    LE_ASSERT(le_sms_SetSmsCenterAddress(smscStrs) == LE_OK);

    // Get current SMS service center address.
    LE_ASSERT(le_sms_GetSmsCenterAddress(smscMdmStr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == LE_OK);

    // Restore previous SMS service center address.
    LE_ASSERT(le_sms_SetSmsCenterAddress(smscMdmRefStr) == LE_OK);

    // check if value get match with value set.
    LE_ASSERT(strncmp(smscStrs,smscMdmStr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Required: At less two SMS with unknown encoding format must be present in the SIM
 * Test: Check that Legato can create a List object that lists the received messages with
 *   unknown encoding format present in the storage area.  Test that messages status can be
 *   changed or these messages can be deleted
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_DecodingReceivedList()
{
    le_sms_MsgRef_t         lMsg1, lMsg2,lMsg3;
    le_sms_MsgListRef_t     receivedList;
    char                    tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    char                    timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    uint8_t                 pdu[LE_SMS_PDU_MAX_BYTES];
    size_t                  length;
    uint16_t                myMessageId = 0;
    uint16_t                myMessageSerialNumber = 0;
    le_sms_ErrorCode_t      rpCause;
    le_sms_ErrorCode_t      tpCause;
    uint8_t                 myMessageReference = 0;
    uint8_t                 myTora = 0;
    uint8_t                 myStatus = 0;

    // List Received messages
    LE_ASSERT(receivedList = le_sms_CreateRxMsgList());

    lMsg1 = le_sms_GetFirst(receivedList);
    LE_ASSERT(lMsg1 != NULL);
    LE_ASSERT(le_sms_GetStatus(lMsg1) == LE_SMS_RX_UNREAD);

    // test readonly field
    LE_ASSERT(le_sms_SetTimeout(lMsg1, 0) == LE_FAULT);
    LE_ASSERT(le_sms_SetDestination(lMsg1, VOID_PATTERN) == LE_NOT_PERMITTED);
    LE_ASSERT(le_sms_SetText(lMsg1, VOID_PATTERN) == LE_NOT_PERMITTED);
    LE_ASSERT(le_sms_SetBinary(lMsg1, BINARY_TEST_PATTERN, 0) == LE_NOT_PERMITTED);
    LE_ASSERT(le_sms_SetPDU(lMsg1, PDU_TEST_PATTERN_7BITS, 0) == LE_NOT_PERMITTED);
    LE_ASSERT(le_sms_SetUCS2(lMsg1, UCS2_TEST_PATTERN, 0) == LE_NOT_PERMITTED);
    LE_ASSERT(le_sms_SendAsync(lMsg1, NULL, NULL) == LE_FAULT);

    LE_ASSERT(le_sms_GetSenderTel(lMsg1, tel, 2) == LE_OVERFLOW);
    LE_ASSERT(le_sms_GetTimeStamp(lMsg1, timestamp, 1) == LE_OVERFLOW);

    LE_ASSERT(le_sms_GetCellBroadcastId(lMsg1, &myMessageId) == LE_FAULT);
    LE_ASSERT(le_sms_GetCellBroadcastSerialNumber(lMsg1, &myMessageSerialNumber) == LE_FAULT);

    LE_ASSERT(LE_FAULT == le_sms_GetTpMr(lMsg1, &myMessageReference));
    LE_ASSERT(LE_FAULT == le_sms_GetTpRa(lMsg1, &myTora, tel, sizeof(tel)));
    LE_ASSERT(LE_FAULT == le_sms_GetTpScTs(lMsg1, timestamp, sizeof(timestamp)));
    LE_ASSERT(LE_FAULT == le_sms_GetTpDt(lMsg1, timestamp, sizeof(timestamp)));
    LE_ASSERT(LE_FAULT == le_sms_GetTpSt(lMsg1, &myStatus));

    length = 1;
    LE_ASSERT(le_sms_GetPDU(lMsg1, pdu, &length) == LE_OVERFLOW);

    // Verify Mark Read functions on Rx message list
    le_sms_MarkRead(lMsg1);
    LE_ASSERT(le_sms_GetStatus(lMsg1) == LE_SMS_RX_READ);

    // Verify Mark Unread functions on Rx message list
    le_sms_MarkUnread(lMsg1);
    LE_ASSERT(le_sms_GetStatus(lMsg1) == LE_SMS_RX_UNREAD);
    LE_ASSERT(le_sms_GetType(lMsg1) == LE_SMS_TYPE_RX);

    le_sms_Get3GPP2ErrorCode(lMsg1);
    le_sms_GetPlatformSpecificErrorCode(lMsg1);
    le_sms_GetErrorCode(lMsg1, &rpCause, &tpCause);
    LE_INFO("rpCause %d, tpCause %d", rpCause, tpCause);

    // le_sms_Delete() API kills client if message belongs in a Rx list.
    LE_INFO("-TEST- Delete Rx message 1 from storage.%p", lMsg1);
    le_sms_DeleteFromStorage(lMsg1);

    lMsg2 = le_sms_GetNext(receivedList);
    LE_ASSERT(lMsg2 != NULL);
    LE_ASSERT(le_sms_GetType(lMsg1) == LE_SMS_TYPE_RX);

    LE_INFO("-TEST- Delete Rx message 2 from storage.%p", lMsg2);
    le_sms_DeleteFromStorage(lMsg2);

    lMsg3 = le_sms_GetNext(receivedList);
    LE_ASSERT(lMsg3 != NULL);
    LE_ASSERT(le_sms_GetType(lMsg1) == LE_SMS_TYPE_RX);

    LE_INFO("-TEST- Delete Rx message 3 from storage.%p", lMsg3);
    le_sms_DeleteFromStorage(lMsg3);

    LE_INFO("-TEST- Delete the ReceivedList");
    le_sms_DeleteList(receivedList);
}


//--------------------------------------------------------------------------------------------------
/*
 * Test le_sms_SetPreferredStorage() and le_sms_GetPreferredStorage() APIs
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_Storage
(
    void
)
{
    le_sms_Storage_t storage;

    LE_ASSERT(le_sms_SetPreferredStorage(LE_SMS_STORAGE_MAX) == LE_FAULT);

    LE_ASSERT(le_sms_SetPreferredStorage(LE_SMS_STORAGE_NV) == LE_OK);

    LE_ASSERT(le_sms_GetPreferredStorage(&storage) == LE_OK);

    LE_ASSERT(storage == LE_SMS_STORAGE_NV);

    LE_ASSERT(le_sms_SetPreferredStorage(LE_SMS_STORAGE_SIM) == LE_OK);

    LE_ASSERT(le_sms_GetPreferredStorage(&storage) == LE_OK);

    LE_ASSERT(storage == LE_SMS_STORAGE_SIM);
}

//--------------------------------------------------------------------------------------------------
/**
 * Synchronize tests
 *
 * @note: timeout is in milliseconds
 */
//--------------------------------------------------------------------------------------------------
static void WaitForSem
(
    le_sem_Ref_t semaphorePtr,
    uint32_t timeout,
    le_result_t expectedResult
)
{
    le_clk_Time_t waitTime;

    waitTime.sec = (time_t)(timeout / 1000);
    waitTime.usec = 0;

    LE_ASSERT(le_sem_WaitWithTimeOut(semaphorePtr, waitTime) == expectedResult);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove sms full storage handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveFullStorageHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppContext_t * appCtxPtr = (AppContext_t*) param1Ptr;

    le_sms_RemoveFullStorageEventHandler( appCtxPtr->statHandler );

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(SmsThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * SmsReceiveHandler: this handler is subcribed by test task and is called on new received sms.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SmsReceiveHandler
(
    le_sms_MsgRef_t messagePtr,   // [IN] Message object received from the modem.
    void*           contextPtr    // [IN] The handler's context.
)
{

    LE_INFO("Message %p, ctx %p", messagePtr, contextPtr);

    // Store message reference
    ReceivedSmsRef = messagePtr;

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(SmsReceiveThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add sms receive handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void AddSmsHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppSmsContext_t * appCtxPtr = (AppSmsContext_t*) param1Ptr;

    // Subscribe to SMS full storage indication handler
    appCtxPtr->rxHdlrRef = le_sms_AddRxMessageHandler(SmsReceiveHandler, param1Ptr);
    LE_ASSERT(appCtxPtr->rxHdlrRef != NULL);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(SmsReceiveThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove sms receive handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveSmsHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppSmsContext_t * appCtxPtr = (AppSmsContext_t*) param1Ptr;

    le_sms_RemoveRxMessageHandler( appCtxPtr->rxHdlrRef );

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(SmsReceiveThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test remove SMS received handler
 *
 * API tested:
 * - le_sms_RemoveRxMessageHandler
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_RemoveSmsReceiveHandler
(
    void
)
{
    int i;
    pa_sms_NewMessageIndication_t  msgPtrNew;
    // Remove handler le_sms_RemoveFullStorageEventHandler to the eventLoop of tasks
    le_event_QueueFunctionToThread( AppSmsReceiveCtx.appSmsReceiveThread ,
                                    RemoveSmsHandler,
                                    &AppSmsReceiveCtx,
                                    NULL );
    // Wait for the tasks
    WaitForSem(SmsReceiveThreadSemaphore, LONG_TIMEOUT, LE_OK);

    // raise events that calls the handler (Simulate sms receive)
    msgPtrNew.msgIndex = 0;                      // Message index
    msgPtrNew.protocol = PA_SMS_PROTOCOL_GSM;    // protocol used
    msgPtrNew.storage  = PA_SMS_STORAGE_NV;      // SMS Storage used

    msgPtrNew.pduLen   = sizeof(PDU_RECEIVE_TEST_PATTERN_7BITS);   // Cell Broadcast PDU len
    for (i=0; i < msgPtrNew.pduLen; i++)
    {
        msgPtrNew.pduCB[i] = PDU_RECEIVE_TEST_PATTERN_7BITS[i];    // Cell Broadcast PDU data
    }
    pa_sms_SetSmsInStorage(&msgPtrNew);

    // Wait for the semaphore timeout to check that handlers are not called
    WaitForSem(SmsReceiveThreadSemaphore, SHORT_TIMEOUT, LE_TIMEOUT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test remove FullStorage handler
 *
 * API tested:
 * - le_sms_RemoveFullStorageEventHandler
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_RemoveFullStorageHandler
(
    void
)
{
    // Remove handler le_sms_RemoveFullStorageEventHandler to the eventLoop of tasks
    le_event_QueueFunctionToThread( AppCtx.appStorageFullThread ,
                                    RemoveFullStorageHandler,
                                    &AppCtx,
                                    NULL );
    // Wait for the tasks
    WaitForSem(SmsThreadSemaphore, LONG_TIMEOUT, LE_OK);

    // Provoke events that calls the handler (Simulate sms full storage notification)
    AppCtx.storage = LE_SMS_STORAGE_SIM;
    pa_sms_SetFullStorageType(SIMU_SMS_STORAGE_SIM);

    // Wait for the semaphore timeout to check that handlers are not called
    WaitForSem(SmsReceiveThreadSemaphore, SHORT_TIMEOUT, LE_TIMEOUT);
}


//--------------------------------------------------------------------------------------------------
/**
 * FullStorageHandler: this handler is subcribed by test task and is called on sms full storage
 * indication.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FullStorageHandler
(
    le_sms_Storage_t  storage,
    void*            contextPtr
)
{
    AppContext_t * appCtxPtr = (AppContext_t*) contextPtr;

    // test storage indication
    LE_ASSERT(appCtxPtr->storage == storage);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(SmsThreadSemaphore);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test task: this function handles the task and run an eventLoop
 *
 */
//--------------------------------------------------------------------------------------------------
static void* AppHandler
(
    void* ctxPtr
)
{
    AppContext_t * appCtxPtr = (AppContext_t*) ctxPtr;

    // Subscribe to SMS full storage indication handler
    appCtxPtr->statHandler = le_sms_AddFullStorageEventHandler(FullStorageHandler, ctxPtr);
    LE_ASSERT(appCtxPtr->statHandler != NULL);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(SmsThreadSemaphore);

    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the test environment:
 * - create a task
 * - create semaphore (to make checkpoints and synchronize test and tasks)
 * - simulate an full storage
 * - check that state handlers are correctly called
 *
 * API tested:
 * - le_sms_AddFullStorageEventHandler
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_FullStorage
(
    void
)
{
    // Create a semaphore to coordinate the test
    SmsThreadSemaphore = le_sem_Create("HandlerSmsFull",0);

    // int app context
    memset(&AppCtx, 0, sizeof(AppContext_t));

    // Start tasks:
    // the thread subcribes to full storage indication handler using
    // le_sms_AddFullStorageEventHandler
    AppCtx.appStorageFullThread = le_thread_Create("appStorageFullThread", AppHandler, &AppCtx);
    le_thread_Start(AppCtx.appStorageFullThread);

    // Wait that the task have started before continuing the test
    WaitForSem(SmsThreadSemaphore, LONG_TIMEOUT, LE_OK);

    // Simulate sms full storage notification with SIM storage
    AppCtx.storage = LE_SMS_STORAGE_SIM;
    pa_sms_SetFullStorageType(SIMU_SMS_STORAGE_SIM);

    // The task has subscribe to full storage event handler:
    // wait the handlers' calls and check result.
    WaitForSem(SmsThreadSemaphore, LONG_TIMEOUT, LE_OK);

    // Simulate sms full storage notification with NV storage
    AppCtx.storage = LE_SMS_STORAGE_NV;
    pa_sms_SetFullStorageType(SIMU_SMS_STORAGE_NV);

    // wait the handlers' calls and check result.
    WaitForSem(SmsThreadSemaphore, LONG_TIMEOUT, LE_OK);

    // Simulate sms full storage notification with error storage
    AppCtx.storage = LE_SMS_STORAGE_MAX;
    pa_sms_SetFullStorageType(SIMU_SMS_STORAGE_ERROR);

    // wait the handlers' calls and check result.
    WaitForSem(SmsThreadSemaphore, LONG_TIMEOUT, LE_OK);

    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(SmsThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * CallbackSendTestHandler: this function handles the task and run an eventLoop
 *
 */
//--------------------------------------------------------------------------------------------------
static void CallbackSendTestHandler
(
    le_sms_MsgRef_t msgRef,
    le_sms_Status_t status,
    void* contextPtr
)
{
    LE_INFO("Message %p, status %d, ctx %p", msgRef, status, contextPtr);

    le_sms_Delete(msgRef);

    if ((void *)SMS_SEND_TEST_NUMBER_TIMOUT == contextPtr)
    {
        LE_ASSERT(LE_SMS_SENDING_TIMEOUT == status);
    }
    else if ((void *)SMS_SEND_TEST_FAILED == contextPtr)
    {
        LE_ASSERT(LE_SMS_SENDING_FAILED == status);
    }
    else
    {
       LE_ASSERT(LE_SMS_SENT == status);
    }

    // Semaphore is used to synchronize the execution of SMS send
    le_sem_Post(SmsSendSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Testle_sms_send: this function handles the different SMS sending
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_Send
(
    void
)
{
    le_sms_MsgRef_t       myMsg;

    // Create a semaphore to coordinate the test
    SmsSendSemaphore = le_sem_Create("HandlerSmsSend",0);
    pa_sms_SetSmsErrCause(LE_OK);

    // test le_sms_SendText()
    le_sms_SendText(DEST_TEST_PATTERN, TEXT_TEST_PATTERN,
                    CallbackSendTestHandler, (void*)( SMS_SEND_TEST_NUMBER_1));
    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);

    le_sms_SendText(DEST_TEST_PATTERN, SHORT_TEXT_TEST_PATTERN,
                    CallbackSendTestHandler, (void*) SMS_SEND_TEST_NUMBER_2);
    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);

    le_sms_SendText(DEST_TEST_PATTERN, LARGE_TEXT_TEST_PATTERN,
                    CallbackSendTestHandler, (void*) SMS_SEND_TEST_NUMBER_3);
    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);

    // test le_sms_SendPdu()
    le_sms_SendPdu(PDU_TEST_PATTERN_7BITS,
                   sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]),
                   CallbackSendTestHandler, (void*) SMS_SEND_TEST_NUMBER_4);
    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);

    le_sms_SendPdu(PDU_TEST_PATTERN_8BITS,
                   sizeof(PDU_TEST_PATTERN_8BITS)/sizeof(PDU_TEST_PATTERN_8BITS[0]),
                   CallbackSendTestHandler, (void*) SMS_SEND_TEST_NUMBER_5);

    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);

    // test le_sms_SetUCS2() , le_sms_Send()
    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);
    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);
    LE_ASSERT(le_sms_SetUCS2(myMsg, UCS2_TEST_PATTERN, sizeof(UCS2_TEST_PATTERN) / 2) == LE_OK);
    // modify set before sending
    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);
    LE_ASSERT(le_sms_SetUCS2(myMsg, UCS2_TEST_PATTERN, sizeof(UCS2_TEST_PATTERN) / 2) == LE_OK);

    LE_ASSERT(le_sms_Send(myMsg) == LE_OK);
     // send again
    LE_ASSERT(le_sms_Send(myMsg) == LE_OK);

    // modify set after sending
    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);
    LE_ASSERT(le_sms_SetUCS2(myMsg, UCS2_TEST_PATTERN, sizeof(UCS2_TEST_PATTERN) / 2) == LE_OK);
    le_sms_Delete(myMsg );

    // test le_sms_SetBinary() , le_sms_SendAsync()
    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);
    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);
    LE_ASSERT(le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(BINARY_TEST_PATTERN)/
                               sizeof(BINARY_TEST_PATTERN[0])) == LE_OK);
    LE_ASSERT(le_sms_SetTimeout(myMsg, 20) == LE_OK);
    LE_ASSERT(le_sms_SendAsync(myMsg,CallbackSendTestHandler,
                               (void*) SMS_SEND_TEST_NUMBER_6) == LE_OK);
    // send again
    LE_ASSERT(le_sms_SendAsync(myMsg,CallbackSendTestHandler,
                               (void*) SMS_SEND_TEST_NUMBER_6) == LE_FAULT);

    // modify set after sending
    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);
    LE_ASSERT(le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(BINARY_TEST_PATTERN)/
                               sizeof(BINARY_TEST_PATTERN[0])) == LE_OK);
    LE_ASSERT(le_sms_SetTimeout(myMsg, 20) == LE_FAULT);

    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);

    // test le_sms_SetPDU()
    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);
    LE_ASSERT(le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, sizeof(PDU_TEST_PATTERN_7BITS)/
                            sizeof(PDU_TEST_PATTERN_7BITS[0])) == LE_OK);
    LE_ASSERT(le_sms_SendAsync(myMsg,CallbackSendTestHandler,
                               (void*) SMS_SEND_TEST_NUMBER_7) == LE_OK);
    // modify set after sending
    LE_ASSERT(le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_8BITS, sizeof(PDU_TEST_PATTERN_8BITS)/
                            sizeof(PDU_TEST_PATTERN_8BITS[0])) == LE_OK);

    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);

    // test le_sms_SetPDU()
    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);
    LE_ASSERT(le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_PTP_DCS_0x10_7BITS,
                            sizeof(PDU_TEST_PATTERN_PTP_DCS_0x10_7BITS)/
                            sizeof(PDU_TEST_PATTERN_PTP_DCS_0x10_7BITS[0])) == LE_OK);
    LE_ASSERT(le_sms_SendAsync(myMsg,CallbackSendTestHandler,
                               (void*) SMS_SEND_TEST_NUMBER_8) == LE_OK);
    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);

    // test le_sms_SetPDU()
    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);
    LE_ASSERT(le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_PTP_DCS_0xC8_7BITS,
                            sizeof(PDU_TEST_PATTERN_PTP_DCS_0xC8_7BITS)/
                            sizeof(PDU_TEST_PATTERN_PTP_DCS_0xC8_7BITS[0])) == LE_OK);
    LE_ASSERT(le_sms_SendAsync(myMsg,CallbackSendTestHandler,
                               (void*) SMS_SEND_TEST_NUMBER_9) == LE_OK);
    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);

    // test le_sms_SetPDU()
    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);
    LE_ASSERT(le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_PTP_DCS_0xC0_7BITS,
                            sizeof(PDU_TEST_PATTERN_PTP_DCS_0xC0_7BITS)/
                            sizeof(PDU_TEST_PATTERN_PTP_DCS_0xC0_7BITS[0])) == LE_OK);
    LE_ASSERT(le_sms_SendAsync(myMsg,CallbackSendTestHandler,
                               (void*) SMS_SEND_TEST_NUMBER_10) == LE_OK);
    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);

    // test error causes
    pa_sms_SetSmsErrCause(LE_TIMEOUT);
    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);
    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);
    LE_ASSERT(le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(BINARY_TEST_PATTERN)/
                               sizeof(BINARY_TEST_PATTERN[0])) == LE_OK);
    LE_ASSERT(le_sms_SendAsync(myMsg,CallbackSendTestHandler,
                               (void*) SMS_SEND_TEST_NUMBER_TIMOUT) == LE_OK);
    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);

    pa_sms_SetSmsErrCause(LE_NOT_POSSIBLE);
    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);
    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);
    LE_ASSERT(le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(BINARY_TEST_PATTERN)/
                               sizeof(BINARY_TEST_PATTERN[0])) == LE_OK);
    LE_ASSERT(le_sms_SendAsync(myMsg,CallbackSendTestHandler,
                               (void*)SMS_SEND_TEST_FAILED) == LE_OK);

    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);
    pa_sms_SetSmsErrCause(LE_OK);

    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(SmsSendSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test task: this function handles the task and run an eventLoop
 *
 */
//--------------------------------------------------------------------------------------------------
static void* AppSmsReceiveHandler
(
    void* ctxPtr
)
{
    AppSmsContext_t * appCtxPtr = (AppSmsContext_t*) ctxPtr;

    // Subscribe to SMS full storage indication handler
    appCtxPtr->rxHdlrRef = le_sms_AddRxMessageHandler(SmsReceiveHandler, ctxPtr);
    LE_ASSERT(appCtxPtr->rxHdlrRef != NULL);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(SmsReceiveThreadSemaphore);

    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the test environment:
 * - create a task
 * - create semaphore (to make checkpoints and synchronize test and tasks)
 * - simulate an full storage
 * - check that state handlers are correctly called
 *
 * API tested:
 * - le_sms_AddFullStorageEventHandler
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_Receive
(
    void
)
{
    pa_sms_NewMessageIndication_t  msgPtrNew;
    int i;

    // Create a semaphore to coordinate the test
    SmsReceiveThreadSemaphore = le_sem_Create("HandlerSmsReceive",0);

    // int app context
    memset(&AppSmsReceiveCtx, 0, sizeof(AppSmsContext_t));

    // Start tasks:
    // the thread subcribes to receive sms handler using le_sms_AddFullStorageEventHandler
    AppSmsReceiveCtx.appSmsReceiveThread = le_thread_Create("SmsReceiveThread",
                                                            AppSmsReceiveHandler,
                                                            &AppSmsReceiveCtx);
    le_thread_Start(AppSmsReceiveCtx.appSmsReceiveThread);

    // Wait that the task have started before continuing the test
    WaitForSem(SmsReceiveThreadSemaphore, LONG_TIMEOUT, LE_OK);

    // test List Received messages empty
    LE_ASSERT(le_sms_CreateRxMsgList() == NULL);

    msgPtrNew.msgIndex = 0;                        // Message index
    msgPtrNew.protocol = PA_SMS_PROTOCOL_GW_CB;    // protocol used
    msgPtrNew.storage  = PA_SMS_STORAGE_NONE;      // SMS Storage used
    // Cell Broadcast PDU len
    msgPtrNew.pduLen   = sizeof(PDU_RECEIVE_TEST_PATTERN_BROADCAST_7BITS);
    for (i=0; i < msgPtrNew.pduLen; i++)
    {
        // Cell Broadcast PDU data
        msgPtrNew.pduCB[i] = PDU_RECEIVE_TEST_PATTERN_BROADCAST_7BITS[i];
    }
    pa_sms_SetSmsInStorage(&msgPtrNew);

    // Wait that the task have started before continuing the test
    WaitForSem(SmsReceiveThreadSemaphore, LONG_TIMEOUT, LE_OK);

    msgPtrNew.msgIndex = 0;    // Message index
    msgPtrNew.protocol = PA_SMS_PROTOCOL_GSM;     // protocol used
    msgPtrNew.storage  = PA_SMS_STORAGE_SIM;      // SMS Storage used
    // Cell Broadcast PDU len
    msgPtrNew.pduLen   = sizeof(PDU_RECEIVE_TEST_PATTERN_SMSPDU_UCS2_16_BITS);
    for (i=0; i < msgPtrNew.pduLen; i++)
    {
        // Cell Broadcast PDU data
        msgPtrNew.pduCB[i] = PDU_RECEIVE_TEST_PATTERN_SMSPDU_UCS2_16_BITS[i];
    }
    pa_sms_SetSmsInStorage(&msgPtrNew);

    // Wait that the task have started before continuing the test
    WaitForSem(SmsReceiveThreadSemaphore, LONG_TIMEOUT, LE_OK);

   // Check that no more call of the semaphore
   LE_ASSERT(le_sem_GetValue(SmsReceiveThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test Testle_sms_CellBroadcast : test SMS CB
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_CellBroadcast
(
    void
)
{
    // test gsm SMS CB
    LE_ASSERT(le_sms_ClearCellBroadcastIds() == LE_OK);
    LE_ASSERT(le_sms_ActivateCellBroadcast() == LE_OK);
    LE_ASSERT(le_sms_DeactivateCellBroadcast() == LE_OK);
    LE_ASSERT(le_sms_AddCellBroadcastIds(0,50) == LE_OK);
    LE_ASSERT(le_sms_AddCellBroadcastIds(0,50) == LE_FAULT);
    LE_ASSERT(le_sms_RemoveCellBroadcastIds(0,100) == LE_FAULT);
    LE_ASSERT(le_sms_RemoveCellBroadcastIds(0,50) == LE_OK);
    LE_ASSERT(le_sms_RemoveCellBroadcastIds(0,50) == LE_FAULT);
    LE_ASSERT(le_sms_AddCellBroadcastIds(60,110) == LE_OK);
    LE_ASSERT(le_sms_ClearCellBroadcastIds() == LE_OK);

    // test cdma SMS CB
    LE_ASSERT(le_sms_ClearCdmaCellBroadcastServices() == LE_OK);
    LE_ASSERT(le_sms_ActivateCdmaCellBroadcast() == LE_OK);
    LE_ASSERT(le_sms_DeactivateCdmaCellBroadcast() == LE_OK);
    LE_ASSERT(le_sms_AddCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_MAX,
                                                  LE_SMS_LANGUAGE_UNKNOWN) == LE_BAD_PARAMETER);
    LE_ASSERT(le_sms_AddCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN,
                                                  LE_SMS_LANGUAGE_MAX) == LE_BAD_PARAMETER);
    LE_ASSERT(le_sms_AddCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN,
                                                  LE_SMS_LANGUAGE_UNKNOWN) == LE_OK);
    LE_ASSERT(le_sms_AddCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN,
                                                  LE_SMS_LANGUAGE_UNKNOWN) == LE_FAULT);
    LE_ASSERT(le_sms_RemoveCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN,
                                                     LE_SMS_LANGUAGE_ENGLISH) == LE_FAULT);
    LE_ASSERT(le_sms_RemoveCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN,
                                                     LE_SMS_LANGUAGE_UNKNOWN) == LE_OK);
    LE_ASSERT(le_sms_RemoveCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN,
                                                     LE_SMS_LANGUAGE_UNKNOWN) == LE_FAULT);
    LE_ASSERT(le_sms_ClearCdmaCellBroadcastServices() == LE_OK);
    LE_ASSERT(le_sms_RemoveCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_MAX+1,
                                                    LE_SMS_LANGUAGE_ENGLISH) == LE_BAD_PARAMETER);
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve message counters and test if the values are as expected
 */
//--------------------------------------------------------------------------------------------------
static void GetAndCheckSmsCounters
(
    SmsCount_t* countPtr,       ///< Actual message counters
    SmsCount_t* expectedPtr     ///< Expected message counters
)
{
    LE_ASSERT_OK(le_sms_GetCount(LE_SMS_TYPE_RX, &countPtr->rx));
    LE_ASSERT(expectedPtr->rx == countPtr->rx);
    LE_ASSERT_OK(le_sms_GetCount(LE_SMS_TYPE_TX, &countPtr->tx));
    LE_ASSERT(expectedPtr->tx == countPtr->tx);
    LE_ASSERT_OK(le_sms_GetCount(LE_SMS_TYPE_BROADCAST_RX, &countPtr->rxCb));
    LE_ASSERT(expectedPtr->rxCb == countPtr->rxCb);
}

//--------------------------------------------------------------------------------------------------
/**
 * Testle_sms_Statistics: this function tests the SMS statistics
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_Statistics
(
    void
)
{
    SmsCount_t count;
    SmsCount_t expected;
    le_sms_MsgRef_t myMsg;
    pa_sms_NewMessageIndication_t msgPtrNew;
    int i;

    memset(&count, 0, sizeof(SmsCount_t));
    memset(&expected, 0, sizeof(SmsCount_t));

    // Reset counters
    le_sms_ResetCount();

    // Check that counters are set to 0
    GetAndCheckSmsCounters(&count, &expected);

    // Start counting
    le_sms_StartCount();

    // Message sent synchronously
    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);
    LE_ASSERT_OK(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN));
    LE_ASSERT_OK(le_sms_SetUCS2(myMsg, UCS2_TEST_PATTERN, sizeof(UCS2_TEST_PATTERN)/2));
    LE_ASSERT_OK(le_sms_Send(myMsg));
    le_sms_Delete(myMsg);
    expected.tx++;
    GetAndCheckSmsCounters(&count, &expected);

    // Message sent asynchronously
    pa_sms_SetSmsErrCause(LE_OK);
    le_sms_SendText(DEST_TEST_PATTERN, TEXT_TEST_PATTERN,
                    CallbackSendTestHandler, (void*)( SMS_SEND_TEST_NUMBER_1));
    WaitForSem(SmsSendSemaphore, LONG_TIMEOUT, LE_OK);
    expected.tx++;
    GetAndCheckSmsCounters(&count, &expected);

    // Add again received SMS handler
    le_event_QueueFunctionToThread(AppSmsReceiveCtx.appSmsReceiveThread,
                                   AddSmsHandler,
                                   &AppSmsReceiveCtx,
                                   NULL);
    WaitForSem(SmsReceiveThreadSemaphore, LONG_TIMEOUT, LE_OK);

    // Broadcast message received
    msgPtrNew.msgIndex = 0;                         // Message index
    msgPtrNew.protocol = PA_SMS_PROTOCOL_GW_CB;     // protocol used
    msgPtrNew.storage  = PA_SMS_STORAGE_NONE;       // SMS Storage used
    msgPtrNew.pduLen   = sizeof(PDU_RECEIVE_TEST_PATTERN_BROADCAST_7BITS);
    for (i=0; i < msgPtrNew.pduLen; i++)
    {
        msgPtrNew.pduCB[i] = PDU_RECEIVE_TEST_PATTERN_BROADCAST_7BITS[i];
    }
    pa_sms_SetSmsInStorage(&msgPtrNew);
    WaitForSem(SmsReceiveThreadSemaphore, LONG_TIMEOUT, LE_OK);
    expected.rxCb++;
    GetAndCheckSmsCounters(&count, &expected);

    // Message received
    msgPtrNew.msgIndex = 0;                         // Message index
    msgPtrNew.protocol = PA_SMS_PROTOCOL_GSM;       // protocol used
    msgPtrNew.storage  = PA_SMS_STORAGE_SIM;        // SMS Storage used
    msgPtrNew.pduLen   = sizeof(PDU_RECEIVE_TEST_PATTERN_SMSPDU_UCS2_16_BITS);
    for (i=0; i < msgPtrNew.pduLen; i++)
    {
        msgPtrNew.pduCB[i] = PDU_RECEIVE_TEST_PATTERN_SMSPDU_UCS2_16_BITS[i];
    }
    pa_sms_SetSmsInStorage(&msgPtrNew);
    WaitForSem(SmsReceiveThreadSemaphore, LONG_TIMEOUT, LE_OK);
    expected.rx++;
    GetAndCheckSmsCounters(&count, &expected);

    // Stop counting
    le_sms_StopCount();

    // Message sent synchronously
    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);
    LE_ASSERT_OK(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN));
    LE_ASSERT_OK(le_sms_SetUCS2(myMsg, UCS2_TEST_PATTERN, sizeof(UCS2_TEST_PATTERN)/2));
    LE_ASSERT_OK(le_sms_Send(myMsg));
    le_sms_Delete(myMsg);
    GetAndCheckSmsCounters(&count, &expected);

    // Start counting again
    le_sms_StartCount();

    // Message sent synchronously
    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);
    LE_ASSERT_OK(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN));
    LE_ASSERT_OK(le_sms_SetUCS2(myMsg, UCS2_TEST_PATTERN, sizeof(UCS2_TEST_PATTERN)/2));
    LE_ASSERT_OK(le_sms_Send(myMsg));
    le_sms_Delete(myMsg);
    expected.tx++;
    GetAndCheckSmsCounters(&count, &expected);

    // Reset counters
    le_sms_ResetCount();

    // Check that counters are set to 0
    expected.rx = 0;
    expected.tx = 0;
    expected.rxCb = 0;
    GetAndCheckSmsCounters(&count, &expected);

    // Check error cases
    LE_ASSERT(LE_BAD_PARAMETER == le_sms_GetCount(LE_SMS_TYPE_BROADCAST_RX + 1, &count.rx));
    LE_ASSERT(LE_BAD_PARAMETER == le_sms_GetCount(LE_SMS_TYPE_BROADCAST_RX, NULL));
}

//--------------------------------------------------------------------------------------------------
/**
 * Testle_sms_StatusReport: this function tests the SMS Status Report
 */
//--------------------------------------------------------------------------------------------------
static void Testle_sms_StatusReport
(
    void
)
{
    int     i;
    bool    statusReportEnabled;
    uint8_t myMessageReference = 0;
    uint8_t myTora = 0;
    char    myRa[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    char    myTimestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    uint8_t myStatus = 0;
    pa_sms_NewMessageIndication_t msgPtrNew;

    // Test SMS Status Report (de)activation
    LE_ASSERT_OK(le_sms_EnableStatusReport());
    LE_ASSERT_OK(le_sms_IsStatusReportEnabled(&statusReportEnabled));
    LE_ASSERT(true == statusReportEnabled);
    LE_ASSERT_OK(le_sms_DisableStatusReport());
    LE_ASSERT_OK(le_sms_IsStatusReportEnabled(&statusReportEnabled));
    LE_ASSERT(false == statusReportEnabled);

    // Test API error cases
    LE_ASSERT(LE_BAD_PARAMETER == le_sms_GetTpMr(NULL, &myMessageReference));
    LE_ASSERT(LE_BAD_PARAMETER == le_sms_GetTpRa(NULL, &myTora, myRa, sizeof(myRa)));
    LE_ASSERT(LE_BAD_PARAMETER == le_sms_GetTpScTs(NULL, myTimestamp, sizeof(myTimestamp)));
    LE_ASSERT(LE_BAD_PARAMETER == le_sms_GetTpDt(NULL, myTimestamp, sizeof(myTimestamp)));
    LE_ASSERT(LE_BAD_PARAMETER == le_sms_GetTpSt(NULL, &myStatus));

    // Simulate reception of a SMS Status Report
    msgPtrNew.msgIndex = 0;                         // Message index
    msgPtrNew.protocol = PA_SMS_PROTOCOL_GSM;       // Protocol used
    msgPtrNew.storage  = PA_SMS_STORAGE_NONE;       // SMS Storage used
    msgPtrNew.pduLen   = sizeof(PDU_RECEIVE_TEST_PATTERN_STATUS_REPORT);
    for (i=0; i < msgPtrNew.pduLen; i++)
    {
        msgPtrNew.pduCB[i] = PDU_RECEIVE_TEST_PATTERN_STATUS_REPORT[i];
    }
    pa_sms_SetSmsInStorage(&msgPtrNew);
    WaitForSem(SmsReceiveThreadSemaphore, LONG_TIMEOUT, LE_OK);

    // Check APIs and received data
    LE_ASSERT(ReceivedSmsRef);
    LE_ASSERT(LE_OVERFLOW == le_sms_GetTpRa(ReceivedSmsRef, &myTora, myRa, 1));
    LE_ASSERT(LE_OVERFLOW == le_sms_GetTpScTs(ReceivedSmsRef, myTimestamp, 1));
    LE_ASSERT(LE_OVERFLOW == le_sms_GetTpDt(ReceivedSmsRef, myTimestamp, 1));
    LE_ASSERT_OK(le_sms_GetTpMr(ReceivedSmsRef, &myMessageReference));
    LE_ASSERT_OK(le_sms_GetTpRa(ReceivedSmsRef, &myTora, myRa, sizeof(myRa)));
    LE_ASSERT_OK(le_sms_GetTpScTs(ReceivedSmsRef, myTimestamp, sizeof(myTimestamp)));
    LE_ASSERT_OK(le_sms_GetTpDt(ReceivedSmsRef, myTimestamp, sizeof(myTimestamp)));
    LE_ASSERT_OK(le_sms_GetTpSt(ReceivedSmsRef, &myStatus));
}

//--------------------------------------------------------------------------------------------------
/**
 * SMS API Unitary Test
 */
//--------------------------------------------------------------------------------------------------
void testle_sms_SmsApiUnitTest
(
    void
)
{
    LE_ASSERT(le_sms_Init() == LE_OK);

    LE_INFO("Test Testle_sms_Storage started");
    Testle_sms_Storage();

    LE_INFO("Test Testle_sms_SetGetSmsCenterAddress started");
    Testle_sms_SetGetSmsCenterAddress();

    LE_INFO("Test Testle_sms_SetGetBinary started");
    Testle_sms_SetGetBinary();

    LE_INFO("Test Testle_sms_SetGetText started");
    Testle_sms_SetGetText();

    LE_INFO("Test Testle_sms_SetGetPDU started");
    Testle_sms_SetGetPDU();

    LE_INFO("Test Testle_sms_SetGetUCS2 started");
    Testle_sms_SetGetUCS2();

    LE_INFO("Test Testle_sms_Send started");
    Testle_sms_Send();

    LE_INFO("Test Testle_sms_CellBroadcast started");
    Testle_sms_CellBroadcast();

    LE_INFO("Test Testle_sms_Receive started");
    Testle_sms_Receive();

    LE_INFO("Test Testle_sms_RemoveSmsReceiveHandler started");
    Testle_sms_RemoveSmsReceiveHandler();

    // note : there are 3 SMS received
    LE_INFO("Test Testle_sms_DecodingReceivedList started");
    Testle_sms_DecodingReceivedList();

    LE_INFO("Test Testle_sms_FullStorage started");
    Testle_sms_FullStorage();

    LE_INFO("Test Testle_sms_RemoveFullStorageHandler started");
    Testle_sms_RemoveFullStorageHandler();

    LE_INFO("Test Testle_sms_Statistics started");
    Testle_sms_Statistics();

    LE_INFO("Test Testle_sms_StatusReport started");
    Testle_sms_StatusReport();

    LE_INFO("smsApiUnitTest sequence PASSED");
}
