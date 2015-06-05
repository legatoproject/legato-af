/**
  * This module implements the le_sms's unit tests.
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include "interfaces.h"
#include <semaphore.h>


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
    size_t                uintval;


    LE_INFO("-TEST- New SMS message received ! msg.%p", msg);
    myformat = le_sms_GetFormat(msg);
    if (myformat == LE_SMS_FORMAT_TEXT)
    {
        res = le_sms_GetSenderTel(msg, tel, 1);
        if(res != LE_OVERFLOW)
        {
            LE_ERROR("-TEST 1/13- Check le_sms_GetSenderTel failure (LE_OVERFLOW expected) !");
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST 1/13- Check le_sms_GetSenderTel passed (LE_OVERFLOW expected).");
        }

        res = le_sms_GetSenderTel(msg, tel, sizeof(tel));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST 2/13- Check le_sms_GetSenderTel failure (LE_OK expected) !");
            LE_ERROR("FAILED !!");
            return;

        }
        else
        {
            LE_INFO("-TEST 2/13- Check le_sms_GetSenderTel passed (%s) (LE_OK expected).", tel);
        }

        if(strncmp(&tel[strlen(tel)-4], &DEST_TEST_PATTERN[strlen(DEST_TEST_PATTERN)-4], 4))
        {
            LE_ERROR("-TEST  3/13- Check le_sms_GetSenderTel, bad Sender Telephone number! (%s)",
                            tel);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  3/13- Check le_sms_GetSenderTel, Sender Telephone number OK.");
        }

        uintval = le_sms_GetUserdataLen(msg);
        if((uintval != strlen(TEXT_TEST_PATTERN)) &&
           (uintval != strlen(SHORT_TEXT_TEST_PATTERN)) &&
           (uintval != strlen(LARGE_TEXT_TEST_PATTERN)))
        {
            LE_ERROR("-TEST  4/13- Check le_sms_GetLen, bad expected text length! (%zd)", uintval);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  4/13- Check le_sms_GetLen OK.");
        }

        res = le_sms_GetTimeStamp(msg, timestamp, 1);
        if(res != LE_OVERFLOW)
        {
            LE_ERROR("-TEST  5/13- Check le_sms_GetTimeStamp -LE_OVERFLOW error- failure!");
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  5/13- Check le_sms_GetTimeStamp -LE_OVERFLOW error- OK.");
        }

        res = le_sms_GetTimeStamp(msg, timestamp, sizeof(timestamp));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  6/13- Check le_sms_GetTimeStamp failure!");
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  6/13- Check le_sms_GetTimeStamp OK (%s).", timestamp);
        }

        res = le_sms_GetText(msg, text, sizeof(text));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  7/13- Check le_sms_GetText failure!");
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  7/13- Check le_sms_GetText OK.");
        }

        if((strncmp(text, TEXT_TEST_PATTERN, strlen(TEXT_TEST_PATTERN)) != 0) &&
           (strncmp(text, SHORT_TEXT_TEST_PATTERN, strlen(SHORT_TEXT_TEST_PATTERN)) != 0) &&
           (strncmp(text, LARGE_TEXT_TEST_PATTERN, strlen(LARGE_TEXT_TEST_PATTERN)) != 0)
        )
        {
            LE_ERROR("-TEST  8/13- Check le_sms_GetText, bad expected received text! (%s)", text);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  8/13- Check le_sms_GetText, received text OK.");
        }

        // Verify that the message is read-only
        res = le_sms_SetDestination(msg, DEST_TEST_PATTERN);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  9/13- Check le_sms_SetDestination, parameter check failure!");
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  9/13- Check le_sms_SetDestination OK.");
        }

        res = le_sms_SetText(msg, TEXT_TEST_PATTERN);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  10/13- Check le_sms_SetText, parameter check failure!");
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  10/13- Check le_sms_SetText OK.");
        }

        // Verify Mark Read/Unread functions
        le_sms_MarkRead(msg);

        mystatus = le_sms_GetStatus(msg);
        if(mystatus != LE_SMS_RX_READ)
        {
            LE_ERROR("-TEST  11/13- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  11/13- Check le_sms_GetStatus, status OK.");
        }

        le_sms_MarkUnread(msg);

        mystatus = le_sms_GetStatus(msg);
        if(mystatus != LE_SMS_RX_UNREAD)
        {
            LE_ERROR("-TEST  12/13- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  12/13- Check le_sms_GetStatus, status OK.");
        }

        res = le_sms_DeleteFromStorage(msg);
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  13/13- Check le_sms_DeleteFromStorage failure!");
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  13/13- Check le_sms_DeleteFromStorage OK.");
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

    le_event_RunLoop();
    return NULL;
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
                            CallbackTestHandler, NULL );
        }
        LE_INFO("-TEST- Create Async text Msg %p", myMsg);
    }

    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Text Message Object Set/Get APIS.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_sms_SetGetText
(
    void
)
{
    le_result_t           res = LE_FAULT;
    le_sms_MsgRef_t       myMsg;
    le_sms_Format_t       myformat;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    char                  tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    char                  text[LE_SMS_TEXT_MAX_BYTES] = {0};
    size_t                uintval;

    myMsg = le_sms_Create();

    if (myMsg)
    {
        res = le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_SetText(myMsg, TEXT_TEST_PATTERN);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        myformat = le_sms_GetFormat(myMsg);
        if (myformat != LE_SMS_FORMAT_TEXT)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_GetSenderTel(myMsg, tel, sizeof(tel));
        if (res != LE_NOT_PERMITTED)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp));
        if (res != LE_NOT_PERMITTED)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        uintval = le_sms_GetUserdataLen(myMsg);
        if (uintval != strlen(TEXT_TEST_PATTERN))
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_GetText(myMsg, text, 1);
        if (res != LE_OVERFLOW)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_GetText(myMsg, text, sizeof(text));
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }
        if (strncmp(text, TEXT_TEST_PATTERN, strlen(TEXT_TEST_PATTERN)) != 0)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_SetDestination(myMsg, VOID_PATTERN);
        if (res != LE_BAD_PARAMETER)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_SetText(myMsg, VOID_PATTERN);
        if (res != LE_BAD_PARAMETER)
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
 * Test: Raw binary Message Object Set/Get APIS.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_sms_SetGetBinary
(
    void
)
{
    le_result_t           res = LE_FAULT;
    le_sms_MsgRef_t       myMsg;
    le_sms_Format_t       myformat;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    char                  tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    uint8_t               raw[LE_SMS_BINARY_MAX_BYTES];
    size_t                uintval;
    uint32_t              i;

    myMsg = le_sms_Create();

    if (myMsg)
    {
        res = le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(BINARY_TEST_PATTERN));
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        myformat = le_sms_GetFormat(myMsg);
        if (myformat != LE_SMS_FORMAT_BINARY)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_GetSenderTel(myMsg, tel, sizeof(tel));
        if (res != LE_NOT_PERMITTED)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp));
        if (res != LE_NOT_PERMITTED)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        uintval = le_sms_GetUserdataLen(myMsg);
        if (uintval != sizeof(BINARY_TEST_PATTERN))
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        uintval=1;
        res = le_sms_GetBinary(myMsg, raw, &uintval);
        if (res != LE_OVERFLOW)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        uintval = sizeof(BINARY_TEST_PATTERN);
        res = le_sms_GetBinary(myMsg, raw, &uintval);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }
        for(i=0; i<sizeof(BINARY_TEST_PATTERN) ; i++)
        {
            if (raw[i] != BINARY_TEST_PATTERN[i])
            {
                le_sms_Delete(myMsg);
                return LE_FAULT;
            }
        }
        if (uintval != sizeof(BINARY_TEST_PATTERN))
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_SetDestination(myMsg, VOID_PATTERN);
        if (res != LE_BAD_PARAMETER)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, 0);
        if (res != LE_BAD_PARAMETER)
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
 * Test: PDU Message Object Set/Get APIS.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_sms_SetGetPDU
(
    void
)
{
    le_result_t           res = LE_FAULT;
    le_sms_MsgRef_t       myMsg;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    char                  tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    uint8_t               pdu[LE_SMS_PDU_MAX_BYTES];
    size_t                uintval;
    uint32_t              i;

    myMsg = le_sms_Create();

    if (myMsg)
    {
        res = le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, (sizeof(PDU_TEST_PATTERN_7BITS)/
                        sizeof(PDU_TEST_PATTERN_7BITS[0])) );
        LE_INFO("le_sms_SetPDU return %d", res);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_GetSenderTel(myMsg, tel, sizeof(tel));
        if (res != LE_NOT_PERMITTED)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp));
        if (res != LE_NOT_PERMITTED)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        uintval = le_sms_GetPDULen(myMsg);
        if (uintval != (sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0])))
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        uintval = 1;
        res = le_sms_GetPDU(myMsg, pdu, &uintval);
        if (res != LE_OVERFLOW)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        uintval = sizeof(pdu);
        res = le_sms_GetPDU(myMsg, pdu, &uintval);
        if (res != LE_OK)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }
        for(i=0; i<sizeof(PDU_TEST_PATTERN_7BITS) ; i++)
        {
            if (pdu[i] != PDU_TEST_PATTERN_7BITS[i])
            {
                le_sms_Delete(myMsg);
                return LE_FAULT;
            }
        }
        if (uintval != sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]))
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        res = le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, 0);
        if (res != LE_BAD_PARAMETER)
        {
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        le_sms_Delete(myMsg);
        res = LE_OK;
    }

    return res;
}

/*
 * Test case le_sms_GetSmsCenterAddress() and le_sms_SetSmsCenterAddress() APIs
 */
static le_result_t Testle_sms_SetGetSmsCenterAddress
(
    void
)
{
    le_result_t           res;
    char smscMdmRefStr[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};
    char smscMdmStr[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};
    char smscStrs[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = "+33123456789";

    // Get current SMS service center address.
    // Check LE_OVERFLOW error case
    res = le_sms_GetSmsCenterAddress(smscMdmRefStr, 5);
    if (res != LE_OVERFLOW)
    {
        return LE_FAULT;
    }

    // Get current SMS service center address.
    res = le_sms_GetSmsCenterAddress(smscMdmRefStr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
    if (res != LE_OK)
    {
        return LE_FAULT;
    }

    // Set "+33123456789" SMS service center address.
    res = le_sms_SetSmsCenterAddress(smscStrs);
    if (res != LE_OK)
    {
        return LE_FAULT;
    }

    // Get current SMS service center address.
    res = le_sms_GetSmsCenterAddress(smscMdmStr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
    if (res != LE_OK)
    {
        return LE_FAULT;
    }

    // Restore previous SMS service center address.
    res = le_sms_SetSmsCenterAddress(smscMdmRefStr);
    if (res != LE_OK)
    {
        return LE_FAULT;
    }

    // check if value get match with value set.
    if (strncmp(smscStrs,smscMdmStr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) != 0)
    {
        return LE_FAULT;
    }

    return LE_OK;
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
        LE_ERROR("Handler not ready !!");
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

        le_sms_Delete(myMsg);

        res = WaitFunction(&SmsRxSynchronization, 10000);
    }

    le_sms_RemoveRxMessageHandler(RxHdlrRef);
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

    NbSmsTx = NB_SMS_ASYNC_TO_SEND;

    // Init the semaphore for asynchronous callback
    sem_init(&SmsTxSynchronization,0,0);

    TxCallBack = le_thread_Create("Tx CallBack", MyTxThread, &pdu_type);
    le_thread_Start(TxCallBack);

    res = WaitFunction(&SmsTxSynchronization, 10000);
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
    if (!myMsg)
    {
        return LE_FAULT;
    }

    res = le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
    if (res != LE_OK)
    {
        le_sms_Delete(myMsg);
        return LE_FAULT;
    }

    res = le_sms_SetText(myMsg, TEXT_TEST_PATTERN);
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

    res = le_sms_Send(myMsg);
    if ((res == LE_FAULT) || (res == LE_FORMAT_ERROR))
    {
        le_sms_Delete(myMsg);
        return LE_FAULT;
    }

    sleep(5);

    // List Received messages
    receivedList = le_sms_CreateRxMsgList();
    if (!receivedList)
    {
        le_sms_Delete(myMsg);
        return LE_FAULT;
    }

    lMsg1 = le_sms_GetFirst(receivedList);
    if (lMsg1 != NULL)
    {
        mystatus = le_sms_GetStatus(lMsg1);
        if ((mystatus != LE_SMS_RX_READ) && (mystatus != LE_SMS_RX_UNREAD))
        {
            le_sms_Delete(myMsg);
            LE_ERROR("- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            return LE_FAULT;
        }
        // le_sms_Delete() API kills client if message belongs in a Rx list.
        LE_INFO("-TEST- Delete Rx message 1 from storage.%p", lMsg1);

        // Verify Mark Read functions on Rx message list
        le_sms_MarkRead(lMsg1);
        mystatus = le_sms_GetStatus(lMsg1);
        if(mystatus != LE_SMS_RX_READ)
        {
            LE_ERROR("- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        // Verify Mark Unread functions on Rx message list
        le_sms_MarkUnread(lMsg1);
        mystatus = le_sms_GetStatus(lMsg1);
        if(mystatus != LE_SMS_RX_UNREAD)
        {
            LE_ERROR("- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }

        // Verify Mark Read functions on Rx message list
        le_sms_MarkRead(lMsg1);
        mystatus = le_sms_GetStatus(lMsg1);
        if(mystatus != LE_SMS_RX_READ)
        {
            LE_ERROR("- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            le_sms_Delete(myMsg);
            return LE_FAULT;
        }
        LE_INFO("-TEST- Delete Rx message 1 from storage.%p", lMsg1);
        le_sms_DeleteFromStorage(lMsg1);
    }
    else
    {
        LE_ERROR("Test requiered at less 2 SMSs in the storage");
        return LE_FAULT;
    }

    lMsg2 = le_sms_GetNext(receivedList);
    if (lMsg2 != NULL)
    {
        mystatus = le_sms_GetStatus(lMsg2);
        if ((mystatus != LE_SMS_RX_READ) && (mystatus != LE_SMS_RX_UNREAD))
        {
            le_sms_Delete(myMsg);
            LE_ERROR("- Check le_sms_GetStatus, bad status (%d)!", mystatus);
            return LE_FAULT;
        }
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

    // Delete sent message
    le_sms_Delete(myMsg);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/*
 * ME must be registered on Network with the SIM in ready state.
 * Test application deletete all Rx SM
 * Check "logread -f | grep sms" log
 * Start app : app start smsTest
 * Execute app : execInApp smsTest smsTest <Phone number>
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_result_t res;
    int i;

    my_struct smstest[] =
    {
          { "le_sms_SetGetSmsCenterAddress()", Testle_sms_SetGetSmsCenterAddress },
          { "le_sms_SetGetText()", Testle_sms_SetGetText },
          { "le_sms_SetGetBinary()", Testle_sms_SetGetBinary },
          { "le_sms_SetGetPDU()", Testle_sms_SetGetPDU },
          { "le_sms_Send_Binary()", Testle_sms_Send_Binary },
          { "le_sms_Send_Text()", Testle_sms_Send_Text },
          //  Test requiered at less 2 SMSs in the storage
          { "le_sms_ReceivedList()", Testle_sms_ReceivedList },
          { "le_sms_AsyncSendText()", Testle_sms_AsyncSendText },
#if PDU_TEST
          // PDU to encode for ASYNC le_sms_SendPdu Async fucntions.
          {   "le_sms_AsyncSendPdu()", Testle_sms_AsyncSendPdu},
          {   "le_sms_Send_Pdu()", Testle_sms_Send_Pdu},
#endif
          { "", NULL }
    };

    if (le_arg_NumArgs() == 1)
    {
        // This function gets the telephone number from the User (interactive case).
        const char* phoneNumber = le_arg_GetArg(0);
        strncpy((char*)DEST_TEST_PATTERN, phoneNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
        LE_INFO("Phone number %s", DEST_TEST_PATTERN);

        // Delete all Rx SMS message
        DeleteMessages();

        for (i=0; smstest[i].ptrfunc != NULL; i++)
        {
            LE_INFO("Test %s STARTED\n", smstest[i].name);
            res =  smstest[i].ptrfunc();
            if (res != LE_OK)
            {
                LE_ERROR("Test %s FAILED\n", smstest[i].name);
                LE_INFO("smsTest sequence FAILED");
                exit(EXIT_FAILURE);
            }
            else
            {
                LE_INFO("Test %s PASSED\n", smstest[i].name);
            }
        }
        LE_INFO("smsTest sequence PASSED");
    }
    else
    {
        LE_ERROR("PRINT USAGE => execInApp smsTest smsTest <SIM Phone Number>");
    }
    // Delete all Rx SMS message
    DeleteMessages();
    exit(EXIT_SUCCESS);
}
