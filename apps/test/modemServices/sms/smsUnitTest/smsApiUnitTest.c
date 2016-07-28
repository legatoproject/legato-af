/**
  * This module implements the le_sms's unit tests.
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "interfaces.h"
#include "pa_sms.h"
#include "pa_simu.h"
#include "pa_sms_simu.h"

#include "le_sms_local.h"

//--------------------------------------------------------------------------------------------------
/**
 * Test sequence Structure list
 */
//--------------------------------------------------------------------------------------------------


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
static uint8_t PDU_TEST_PATTERN_7BITS[]=
                           {0x00,0x01,0x00,0x0A,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x11,0xD4,0x32,0x9E,0x0E,0xA2,0x96,0xE7,0x74,0x10,0x3C,0x4C,
                            0xA7,0x97,0xE5,0x6E};

static uint8_t BINARY_TEST_PATTERN[]={0x05,0x01,0x00,0x0A};

static uint16_t UCS2_TEST_PATTERN[] =
    {   0x4900, 0x7400, 0x2000, 0x6900, 0x7300, 0x2000, 0x7400, 0x6800,
        0x6500, 0x2000, 0x5600, 0x6F00, 0x6900, 0x6300, 0x6500, 0x2000,
        0x2100, 0x2100, 0x2100, 0x2000, 0x4100, 0x7200, 0x6500, 0x2000,
        0x7900, 0x6f00, 0x7500, 0x2000, 0x7200, 0x6500, 0x6100, 0x6400,
        0x7900, 0x2000
    };

static char DEST_TEST_PATTERN[] = "0123456789";

// Task context
typedef struct
{
    le_thread_Ref_t appStorageFullThread;
    le_sms_FullStorageEventHandlerRef_t statHandler;
    le_sms_Storage_t storage;
} AppContext_t;

static AppContext_t AppCtx;
static le_sem_Ref_t SmsThreadSemaphore;
static le_clk_Time_t TimeToWait ={ 0, 1000000 };

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

    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);

    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);

    LE_ASSERT(le_sms_SetText(myMsg, TEXT_TEST_PATTERN) == LE_OK);

    LE_ASSERT(le_sms_GetFormat(myMsg) == LE_SMS_FORMAT_TEXT);

    LE_ASSERT(le_sms_GetSenderTel(myMsg, tel, sizeof(tel)) == LE_NOT_PERMITTED);

    LE_ASSERT(le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp)) == LE_NOT_PERMITTED);

    LE_ASSERT(le_sms_GetUserdataLen(myMsg) == strlen(TEXT_TEST_PATTERN));

    LE_ASSERT(le_sms_GetText(myMsg, text, 1) == LE_OVERFLOW);

    LE_ASSERT(le_sms_GetText(myMsg, text, sizeof(text)) == LE_OK);

    LE_ASSERT(strncmp(text, TEXT_TEST_PATTERN, strlen(TEXT_TEST_PATTERN)) == 0);

    LE_ASSERT(le_sms_SetDestination(myMsg, VOID_PATTERN) == LE_BAD_PARAMETER);

    LE_ASSERT(le_sms_SetText(myMsg, VOID_PATTERN) == LE_BAD_PARAMETER);

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
    size_t                uintval;
    uint32_t              i;

    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);

    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);

    LE_ASSERT(le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, (sizeof(PDU_TEST_PATTERN_7BITS)/
                     sizeof(PDU_TEST_PATTERN_7BITS[0])) ) == LE_OK);

    LE_ASSERT(le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(BINARY_TEST_PATTERN)) == LE_OK);

    LE_ASSERT(le_sms_GetFormat(myMsg) == LE_SMS_FORMAT_BINARY);

    LE_ASSERT(le_sms_GetSenderTel(myMsg, tel, sizeof(tel)) == LE_NOT_PERMITTED);

    LE_ASSERT(le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp)) == LE_NOT_PERMITTED);

    uintval = le_sms_GetUserdataLen(myMsg);
    LE_ASSERT(uintval == sizeof(BINARY_TEST_PATTERN));

    uintval=1;
    LE_ASSERT(le_sms_GetBinary(myMsg, raw, &uintval) == LE_OVERFLOW);

    uintval = sizeof(BINARY_TEST_PATTERN);
    LE_ASSERT(le_sms_GetBinary(myMsg, raw, &uintval) == LE_OK);

    for(i=0; i<sizeof(BINARY_TEST_PATTERN) ; i++)
    {
        LE_ASSERT(raw[i] == BINARY_TEST_PATTERN[i]);
    }

    LE_ASSERT(uintval == sizeof(BINARY_TEST_PATTERN));

    LE_ASSERT(le_sms_SetDestination(myMsg, VOID_PATTERN) == LE_BAD_PARAMETER);

    LE_ASSERT(le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, 0) == LE_BAD_PARAMETER);

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
    size_t                uintval;
    uint32_t              i;

    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);

    LE_ASSERT(le_sms_SetDestination(myMsg, DEST_TEST_PATTERN) == LE_OK);

    LE_ASSERT(le_sms_SetUCS2(myMsg, UCS2_TEST_PATTERN, sizeof(UCS2_TEST_PATTERN) / 2) == LE_OK);

    LE_ASSERT(le_sms_GetFormat(myMsg) == LE_SMS_FORMAT_UCS2);

    LE_ASSERT(le_sms_GetSenderTel(myMsg, tel, sizeof(tel)) == LE_NOT_PERMITTED);

    LE_ASSERT(le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp)) == LE_NOT_PERMITTED);

    uintval = le_sms_GetUserdataLen(myMsg);
    LE_ASSERT(uintval == (sizeof(UCS2_TEST_PATTERN) / 2));

    uintval = 1;
    LE_ASSERT(le_sms_GetUCS2(myMsg, ucs2Raw, &uintval) == LE_OVERFLOW);

    uintval = sizeof(UCS2_TEST_PATTERN);
    LE_ASSERT(le_sms_GetUCS2(myMsg, ucs2Raw, &uintval) == LE_OK);

    for(i=0; i < (sizeof(UCS2_TEST_PATTERN)/2); i++)
    {
        LE_ERROR_IF(ucs2Raw[i] != UCS2_TEST_PATTERN[i],
            "%d/%d 0x%04X==0x%04X", (int) i, (int) sizeof(UCS2_TEST_PATTERN)/2,
            ucs2Raw[i],
            UCS2_TEST_PATTERN[i]);
        LE_ASSERT(ucs2Raw[i] == UCS2_TEST_PATTERN[i]);
    }

    LE_ASSERT(uintval == sizeof(UCS2_TEST_PATTERN) / 2);

    LE_ASSERT(le_sms_SetDestination(myMsg, VOID_PATTERN) == LE_BAD_PARAMETER);

    LE_ASSERT(le_sms_SetUCS2(myMsg, UCS2_TEST_PATTERN, 0) == LE_BAD_PARAMETER);

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
    size_t                uintval;
    uint32_t              i;

    myMsg = le_sms_Create();
    LE_ASSERT(myMsg);

    LE_ASSERT(le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, (sizeof(PDU_TEST_PATTERN_7BITS)/
                    sizeof(PDU_TEST_PATTERN_7BITS[0]))) == LE_OK);

    LE_ASSERT(le_sms_GetSenderTel(myMsg, tel, sizeof(tel)) == LE_NOT_PERMITTED);

    LE_ASSERT(le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp)) == LE_NOT_PERMITTED);

    uintval = le_sms_GetPDULen(myMsg);
    LE_ASSERT(uintval == (sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0])));

    uintval = 1;
    LE_ASSERT(le_sms_GetPDU(myMsg, pdu, &uintval) == LE_OVERFLOW);

    uintval = sizeof(pdu);
    LE_ASSERT(le_sms_GetPDU(myMsg, pdu, &uintval) == LE_OK);

    for(i=0; i<sizeof(PDU_TEST_PATTERN_7BITS) ; i++)
    {
        LE_ASSERT(pdu[i] == PDU_TEST_PATTERN_7BITS[i]);
    }

    LE_ASSERT(uintval == sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]));

    LE_ASSERT(le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, 0) == LE_BAD_PARAMETER);

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
static void Testle_sms_ErrorDecodingReceivedList()
{
    le_sms_MsgRef_t         lMsg1, lMsg2;
    le_sms_MsgListRef_t     receivedList;

    // List Received messages
    receivedList = le_sms_CreateRxMsgList();

    if (receivedList)
    {
        lMsg1 = le_sms_GetFirst(receivedList);
        LE_ASSERT(lMsg1 != NULL);
        LE_ASSERT(le_sms_GetStatus(lMsg1) == LE_SMS_RX_READ);

        // le_sms_Delete() API kills client if message belongs in a Rx list.
        LE_INFO("-TEST- Delete Rx message 1 from storage.%p", lMsg1);

        // Verify Mark Read functions on Rx message list
        le_sms_MarkRead(lMsg1);
        LE_ASSERT(le_sms_GetStatus(lMsg1) == LE_SMS_RX_READ);

        // Verify Mark Unread functions on Rx message list
        le_sms_MarkUnread(lMsg1);
        LE_ASSERT(le_sms_GetStatus(lMsg1) == LE_SMS_RX_UNREAD);

        LE_INFO("-TEST- Delete Rx message 1 from storage.%p", lMsg1);

        le_sms_DeleteFromStorage(lMsg1);
        lMsg2 = le_sms_GetNext(receivedList);
        LE_ASSERT(lMsg2 != NULL);
        le_sms_DeleteFromStorage(lMsg2);
        LE_INFO("-TEST- Delete the ReceivedList");
        le_sms_DeleteList(receivedList);
    }
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
 * Synchronize test thread (i.e. Testle_sms_FullStorage) and task
 *
 */
//--------------------------------------------------------------------------------------------------
static void SynchTest( void )
{
    LE_ASSERT(le_sem_WaitWithTimeOut(SmsThreadSemaphore, TimeToWait) == LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove sms full storage handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveHandler
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
 * Test remove handler
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
                                    RemoveHandler,
                                    &AppCtx,
                                    NULL );
    // Wait for the tasks
    SynchTest();

    // Provoke events that calls the handler (Simulate sms full storage notification)
    AppCtx.storage = LE_SMS_STORAGE_SIM;
    pa_sms_SetFullStorageType(SIMU_SMS_STORAGE_SIM);

    // Wait for the semaphore timeout to check that handlers are not called
    LE_ASSERT( le_sem_WaitWithTimeOut(SmsThreadSemaphore, TimeToWait) == LE_TIMEOUT );
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
    // the thread subcribes to full storage indication handler using le_sms_AddFullStorageEventHandler
    AppCtx.appStorageFullThread = le_thread_Create("appStorageFullThread", AppHandler, &AppCtx);
    le_thread_Start(AppCtx.appStorageFullThread);

    // Wait that the task have started before continuing the test
    SynchTest();

    // Simulate sms full storage notification with SIM storage
    AppCtx.storage = LE_SMS_STORAGE_SIM;
    pa_sms_SetFullStorageType(SIMU_SMS_STORAGE_SIM);

    // The task has subscribe to full storage event handler:
    // wait the handlers' calls and check result.
    SynchTest();

    // Simulate sms full storage notification with NV storage
    AppCtx.storage = LE_SMS_STORAGE_NV;
    pa_sms_SetFullStorageType(SIMU_SMS_STORAGE_NV);

    // wait the handlers' calls and check result.
    SynchTest();

    // Simulate sms full storage notification with error storage
    AppCtx.storage = LE_SMS_STORAGE_MAX;
    pa_sms_SetFullStorageType(SIMU_SMS_STORAGE_ERROR);

    // wait the handlers' calls and check result.
    SynchTest();

    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(SmsThreadSemaphore) == 0);
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

    LE_INFO("Test Testle_sms_ErrorDecodingReceivedList started");
    Testle_sms_ErrorDecodingReceivedList();

    LE_INFO("Test Testle_sms_FullStorage started");
    Testle_sms_FullStorage();

    LE_INFO("Test Testle_sms_RemoveFullStorageHandler started");
    Testle_sms_RemoveFullStorageHandler();

    LE_INFO("smsApiUnitTest sequence PASSED");
}
