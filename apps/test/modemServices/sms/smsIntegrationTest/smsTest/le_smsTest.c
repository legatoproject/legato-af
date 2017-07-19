/**
  * This module implements the le_sms's unit tests.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"
#include <semaphore.h>

/*
 * PDU_TEST flag requests to encode yourself two valid PDUs defined by:
 * - PDU_TEST_PATTERN_8BITS
 * - PDU_TEST_PATTERN_7BITS
 **/
//#define PDU_TEST 1


//--------------------------------------------------------------------------------------------------
/**
 * Test sequence Structure list
 */
//--------------------------------------------------------------------------------------------------

typedef le_result_t (*TestFunc)(void);
typedef struct
{
        char * name;
        TestFunc ptrfunc;
} my_struct;


#define VOID_PATTERN  ""

#define SHORT_TEXT_TEST_PATTERN  "Short"
#define LARGE_TEXT_TEST_PATTERN  "Large Text Test pattern Large Text Test pattern Large Text" \
     " Test pattern Large Text Test pattern Large Text Test pattern Large Text Test patt"
#define TEXT_TEST_PATTERN        "Text Test pattern"

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


#define FAIL_TEXT_TEST_PATTERN  "Fail Text Test pattern Fail Text Test pattern Fail Text Test" \
    " pattern Fail Text Test pattern Fail Text Test pattern Fail Text Test pattern Fail" \
    " Text Test pattern Text Test pattern "

#define NB_SMS_ASYNC_TO_SEND   5
/*
 * Pdu message can be created with this link http://www.smartposition.nl/resources/sms_pdu.html
 */
#if PDU_TEST
static uint8_t PDU_TEST_PATTERN_8BITS[]=
                           {0x00,0x01,0x00,0x0A,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x11,0x54,0x65,0x78,0x74,0x20,0x54,0x65,0x73,0x74,0x20,0x70,
                            0x61,0x74,0x74,0x65,0x72,0x6E};
#endif

/*
 * Pdu message can be created with this link http://www.smartposition.nl/resources/sms_pdu.html
 */
static uint8_t PDU_TEST_PATTERN_7BITS[]=
                           {0x00,0x01,0x00,0x0A,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x11,0xD4,0x32,0x9E,0x0E,0xA2,0x96,0xE7,0x74,0x10,0x3C,0x4C,
                            0xA7,0x97,0xE5,0x6E};

static uint8_t BINARY_TEST_PATTERN[]={0x05,0x01,0x00,0x0A};


static le_sms_RxMessageHandlerRef_t RxHdlrRef = NULL;
static le_sms_FullStorageEventHandlerRef_t FullStorageHdlrRef = NULL;


static char DEST_TEST_PATTERN[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

static sem_t SmsRxSynchronization;
static sem_t SmsTxSynchronization;

static le_thread_Ref_t  RxThread = NULL;
static le_thread_Ref_t  TxCallBack = NULL;

static uint32_t NbSmsRx;
static uint32_t NbSmsTx;

//--------------------------------------------------------------------------------------------------
//                                       Test Functions
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
//--------------------- -----------------------------------------------------------------------------
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


static void CallbackTestHandlerTimeout
(
    le_sms_MsgRef_t msgRef,
    le_sms_Status_t status,
    void* contextPtr
)
{
    LE_INFO("Message %p, status %d, ctx %p, wait %d", msgRef, status, contextPtr, NbSmsTx);

    switch(NbSmsTx)
    {
        case 3:
        {
            if (status != LE_SMS_SENDING_TIMEOUT)
            {
                LE_ERROR("Test 2/4 FAILED");
                NbSmsTx = 5;
            }
            else
            {
                LE_INFO("First result LE_SMS_SENDING_TIMEOUT received");
                LE_INFO("Test 2/4 PASSED");
                NbSmsTx--;
            }
        }
        break;
        case 2:
        {
            if (status != LE_SMS_SENT)
            {
                LE_ERROR("Test 3/4 FAILED");
                NbSmsTx = 5;
            }
            else
            {
                LE_INFO("Second result LE_SMS_SENT received");
                LE_INFO("Test 3/4 PASSED");
                NbSmsTx--;
            }
        }
        break;

        case 1:
        {
            if (status != LE_SMS_SENDING_TIMEOUT)
            {
                LE_ERROR("Test 4/4 FAILED");
                NbSmsTx = 5;
            }
            else
            {
                LE_INFO("Second result LE_SMS_SENDING_TIMEOUT received");
                LE_INFO("Test 4/4 PASSED");
                NbSmsTx--;
            }
        }
        break;

        default:
        {
            LE_ERROR("Unexpected NbSmsTx value %d", NbSmsTx);
        }
        break;
    }

    le_sms_Delete(msgRef);
    if (NbSmsTx == 0)
    {
        sem_post(&SmsTxSynchronization);
    }
}


static void CallbackTestHandler
(
    le_sms_MsgRef_t msgRef,
    le_sms_Status_t status,
    void* contextPtr
)
{
    LE_INFO("Message %p, status %d, ctx %p", msgRef, status, contextPtr);
    le_sms_Delete(msgRef);
    LE_ERROR_IF(status != LE_SMS_SENT, "Test FAILED")
    NbSmsTx--;
    LE_INFO("Number of callback event remaining %d", NbSmsTx);
    if (NbSmsTx == 0)
    {
        sem_post(&SmsTxSynchronization);
    }
}

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
            myMsg = le_sms_SendPdu(PDU_TEST_PATTERN_7BITS,
                            sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]),
                            CallbackTestHandler, (void*) 1);
        }
        else
        {
            myMsg = le_sms_SendText(DEST_TEST_PATTERN, TEXT_TEST_PATTERN,
                            CallbackTestHandler, NULL);
        }
        LE_INFO("-TEST- Create Async text Msg %p", myMsg);

        myMsg = le_sms_Create();

        if (pdu_type)
        {
            le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS,
                            sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]));
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

static le_result_t LeSmsSendPduTime
(
    const uint8_t* pduPtr,
    ///< [IN]
    ///< PDU message.

    size_t pduNumElements,
    ///< [IN]

    le_sms_CallbackResultFunc_t handlerPtr,
    ///< [IN]

    void* contextPtr,

    uint32_t timeout
    ///< [IN]
)
{
    le_result_t res = LE_FAULT;
    le_sms_MsgRef_t myMsg = le_sms_Create();

    if(myMsg == NULL)
    {
        return res;
    }

    res = le_sms_SetPDU(myMsg, pduPtr, pduNumElements);
    if(res != LE_OK)
    {
        le_sms_Delete(myMsg);
        return res;
    }

    res = le_sms_SetTimeout(myMsg, timeout);
    if(res != LE_OK)
    {
        le_sms_Delete(myMsg);
        return res;
    }

    res = le_sms_SendAsync(myMsg, handlerPtr, contextPtr);
    if(res != LE_OK)
    {
        le_sms_Delete(myMsg);
        return res;
    }

    return res;
}

static le_result_t LeSmsSendtextTime
(
      const char* destStr,
          ///< [IN]
          ///< Telephone number string.

      const char* textStr,
          ///< [IN]
          ///< SMS text.

      le_sms_CallbackResultFunc_t handlerPtr,
          ///< [IN]

      void* contextPtr,
          ///< [IN]

      uint32_t timeout
      ///< [IN]
)
{
    le_result_t res = LE_FAULT;
    le_sms_MsgRef_t myMsg = le_sms_Create();

    if(myMsg == NULL)
    {
        return res;
    }

    res = le_sms_SetDestination(myMsg, destStr);
    if(res != LE_OK)
    {
        le_sms_Delete(myMsg);
        return res;
    }

    res = le_sms_SetText(myMsg, textStr);
    if(res != LE_OK)
    {
        le_sms_Delete(myMsg);
        return res;
    }

    res = le_sms_SetTimeout(myMsg, timeout);
    if(res != LE_OK)
    {
        le_sms_Delete(myMsg);
        return res;
    }

    res = le_sms_SendAsync(myMsg, handlerPtr, contextPtr);
    if(res != LE_OK)
    {
        le_sms_Delete(myMsg);
        return res;
    }

    return res;
}

static void* MyTxThreadTimeout
(
    void* context   ///< See parameter documentation above.
)
{
    le_sms_ConnectService();
    bool pdu_type = *((bool *) context);

    le_result_t res = LE_FAULT;

    if (pdu_type)
    {
        res = LeSmsSendPduTime(PDU_TEST_PATTERN_7BITS,
                        sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]),
                        CallbackTestHandlerTimeout, (void*) 1, 0);
        if (res == LE_OK)
        {
            LE_ERROR("Test 1/4 FAILED");
            return NULL;
        }
        LE_INFO("Test 1/4 PASSED");

        res = LeSmsSendPduTime(PDU_TEST_PATTERN_7BITS,
                        sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]),
                        CallbackTestHandlerTimeout, (void*) 1, 1);
        if (res != LE_OK)
        {
            LE_ERROR("Test 2/4 FAILED");
            return NULL;
        }
        LE_INFO("Test 2/4 STARTED");


        res = LeSmsSendPduTime(PDU_TEST_PATTERN_7BITS,
                        sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]),
                        CallbackTestHandlerTimeout, (void*) 1, 20);
        if (res == LE_OK)
        {
            LE_ERROR("Test 3/4 FAILED");
            return NULL;
        }
        LE_INFO("Test 3/4 STARTED");

        res = LeSmsSendPduTime(PDU_TEST_PATTERN_7BITS,
                        sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]),
                        CallbackTestHandlerTimeout, (void*) 1, 1);
        if (res != LE_OK)
        {
            LE_ERROR("Test 4/4 FAILED");
            return NULL;
        }
        LE_INFO("Test 4/4 STARTED");
    }
    else
    {
        res = LeSmsSendtextTime(DEST_TEST_PATTERN, TEXT_TEST_PATTERN,
                        CallbackTestHandlerTimeout, NULL, 0);
        if (res == LE_OK)
        {
            LE_ERROR("Test 1/4 FAILED");
            return NULL;
        }
        LE_INFO("Test 1/4 PASSED");

        res = LeSmsSendtextTime(DEST_TEST_PATTERN, TEXT_TEST_PATTERN,
                        CallbackTestHandlerTimeout, NULL, 1);
        if (res != LE_OK)
        {
            LE_ERROR("Test 2/4 FAILED");
            return NULL;
        }
        LE_INFO("Test 2/4 STARTED");

        res = LeSmsSendtextTime(DEST_TEST_PATTERN, TEXT_TEST_PATTERN,
                        CallbackTestHandlerTimeout, NULL, 20 );
        if (res != LE_OK)
        {
            LE_ERROR("Test 3/4 FAILED");
            return NULL;
        }
        LE_INFO("Test 3/4 STARTED");

        res = LeSmsSendtextTime(DEST_TEST_PATTERN, TEXT_TEST_PATTERN,
                        CallbackTestHandlerTimeout, NULL, 1 );
        if (res != LE_OK)
        {
            LE_ERROR("Test 4/4 FAILED");
            return NULL;
        }
        LE_INFO("Test 4/3 STARTED");
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
static le_result_t Testle_sms_AsyncSendTextTimeout
(
    void
)
{
    le_result_t res;
    bool pdu_type = false;

    NbSmsTx = 3;

    // Init the semaphore for asynchronous callback
    sem_init(&SmsTxSynchronization,0,0);

    TxCallBack = le_thread_Create("Tx CallBack", MyTxThreadTimeout, &pdu_type);
    le_thread_Start(TxCallBack);

    res = WaitFunction(&SmsTxSynchronization, 120000);
    LE_ERROR_IF(res != LE_OK, "SYNC FAILED");

    le_thread_Cancel(TxCallBack);

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

#if PDU_TEST
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
    sem_init(&SmsTxSynchronization,0,0);

    TxCallBack = le_thread_Create("Tx CallBack", MyTxThread, &pdu_type);
    le_thread_Start(TxCallBack);

    res = WaitFunction(&SmsTxSynchronization, 10000);
    le_thread_Cancel(TxCallBack);

    return res;
}
#endif

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

#if PDU_TEST
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
    le_result_t           res = LE_FAULT;
    le_sms_MsgRef_t      myMsg;

    myMsg = le_sms_Create();
    if (myMsg)
    {
        LE_DEBUG("Create Msg %p", myMsg);

        res = le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, sizeof(PDU_TEST_PATTERN_7BITS)/
                        sizeof(PDU_TEST_PATTERN_7BITS[0]));
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

        res = le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_8BITS, sizeof(PDU_TEST_PATTERN_8BITS)/
                        sizeof(PDU_TEST_PATTERN_8BITS[0]));
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
#endif


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
            } while (myMsg != NULL);
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

        LE_ASSERT(Testle_sms_Storage() == LE_OK);

        LE_ASSERT(Testle_sms_AsyncSendTextTimeout() == LE_OK);

        LE_ASSERT(Testle_sms_Send_Binary() == LE_OK);

        LE_ASSERT(Testle_sms_Send_Text() == LE_OK);

        LE_ASSERT(Testle_sms_ReceivedList() == LE_OK);

        LE_ASSERT(Testle_sms_AsyncSendText() == LE_OK);

#if PDU_TEST
        LE_ASSERT(Testle_sms_AsyncSendPdu() == LE_OK);

        LE_ASSERT(Testle_sms_Send_Pdu() == LE_OK);
#endif

        LE_ASSERT(Testle_sms_Send_UCS2() == LE_OK);

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
