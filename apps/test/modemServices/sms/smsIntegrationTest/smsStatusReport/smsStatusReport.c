 /**
  * Integration test for SMS Status Report.
  *
  * This application tests the SMS Status Report feature:
  * - Disable SMS Status Report and send a SMS
  * - Check that no SMS Status Report is received
  * - Enable SMS Status Report and send a SMS
  * - Check that a SMS Status Report is received
  *
  * The SMS Status Report test is run with:
  * @verbatim
    app runProc smsStatusReport --exe=smsStatusReport -- <Destination Number>
    @endverbatim
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Semaphore timeout in seconds
 */
//--------------------------------------------------------------------------------------------------
#define SEMAPHORE_TIMEOUT   10

//--------------------------------------------------------------------------------------------------
/**
 * Destination number for sent SMS
 */
//--------------------------------------------------------------------------------------------------
static char DestinationNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

//--------------------------------------------------------------------------------------------------
/**
 * Thread sync semaphore reference
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t SyncSemRef;

//--------------------------------------------------------------------------------------------------
/**
 * SMS handler reference
 */
//--------------------------------------------------------------------------------------------------
static le_sms_RxMessageHandlerRef_t HandlerRef;

//--------------------------------------------------------------------------------------------------
/**
 * Latest received SMS reference
 */
//--------------------------------------------------------------------------------------------------
static le_sms_MsgRef_t ReceivedSmsRef;


//--------------------------------------------------------------------------------------------------
/**
 * Synchronize tests
 *
 * @note Timeout is in seconds
 */
//--------------------------------------------------------------------------------------------------
static void WaitForSem
(
    le_sem_Ref_t semaphorePtr,  ///< Semaphore reference
    uint32_t timeout,           ///< Timeout in seconds
    le_result_t expectedResult  ///< Expected result
)
{
    le_clk_Time_t waitTime;

    waitTime.sec = timeout;
    waitTime.usec = 0;

    LE_ASSERT(expectedResult == le_sem_WaitWithTimeOut(semaphorePtr, waitTime));
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler to receive SMS
 */
//--------------------------------------------------------------------------------------------------
static void RxSmsHandler
(
    le_sms_MsgRef_t messagePtr,   ///< Message object received from the modem.
    void*           contextPtr    ///< The handler's context.
)
{
    LE_INFO("Message %p, ctx %p", messagePtr, contextPtr);

    // Store message reference
    ReceivedSmsRef = messagePtr;

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(SyncSemRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread to receive SMS
 */
//--------------------------------------------------------------------------------------------------
static void* MyRxThread
(
    void* ctxPtr    ///< Context pointer.
)
{
    le_sms_ConnectService();

    HandlerRef = le_sms_AddRxMessageHandler(RxSmsHandler, ctxPtr);
    LE_ASSERT(HandlerRef);

    le_sem_Post(SyncSemRef);

    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread destructor
 */
//--------------------------------------------------------------------------------------------------
static void MyThreadDestructor
(
    void* ctxPtr    ///< Context pointer.
)
{
    le_sms_RemoveRxMessageHandler(HandlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Main of the test
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_thread_Ref_t rxThread;
    bool statusReportEnabled;
    le_sms_MsgRef_t myMsg;
    uint8_t oldMessageReference = 0;
    uint8_t txMessageReference = 0;
    uint8_t rxMessageReference = 0;
    uint8_t tora = 0;
    char    ra[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    char    timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    uint8_t status = 0;

    LE_INFO("=== Start of SMS Status Report test ===");

    if (1 != le_arg_NumArgs())
    {
        LE_ERROR("Usage: app runProc smsStatusReport --exe=smsStatusReport -- <Dest Number>");
        exit(EXIT_FAILURE);
    }

    const char* phoneNumber = le_arg_GetArg(0);
    if (!phoneNumber)
    {
        LE_ERROR("Destination number is NULL!");
        exit(EXIT_FAILURE);
    }
    strncpy(DestinationNumber, phoneNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
    LE_INFO("Destination number: %s", DestinationNumber);

    // Create semaphore to synchronize threads
    SyncSemRef = le_sem_Create("Thread Sync Sem", 0);

    // Start SMS reception thread
    rxThread = le_thread_Create("SMS reception thread", MyRxThread, NULL);
    le_thread_AddDestructor(MyThreadDestructor, rxThread);
    le_thread_Start(rxThread);

    // Wait for thread start
    WaitForSem(SyncSemRef, SEMAPHORE_TIMEOUT, LE_OK);

    // Do not request a SMS Status Report
    LE_INFO("Disable SMS Status Report");
    LE_ASSERT_OK(le_sms_DisableStatusReport());
    LE_ASSERT_OK(le_sms_IsStatusReportEnabled(&statusReportEnabled));
    LE_ASSERT(false == statusReportEnabled);

    // Send a message
    LE_INFO("Send a SMS");
    LE_ASSERT(myMsg = le_sms_Create());
    LE_ASSERT_OK(le_sms_SetDestination(myMsg, DestinationNumber));
    LE_ASSERT_OK(le_sms_SetText(myMsg, "Do not send a SMS Status Report!"));
    LE_ASSERT_OK(le_sms_Send(myMsg));
    LE_ASSERT_OK(le_sms_GetTpMr(myMsg, &txMessageReference));
    LE_INFO("Message sent with reference: %d", txMessageReference);
    oldMessageReference = txMessageReference;
    le_sms_Delete(myMsg);

    // Wait to check that no Status Report is received
    LE_INFO("Check that no SMS Status Report is received");
    WaitForSem(SyncSemRef, SEMAPHORE_TIMEOUT, LE_TIMEOUT);

    // Request a SMS Status Report
    LE_INFO("Enable SMS Status Report");
    LE_ASSERT_OK(le_sms_EnableStatusReport());
    LE_ASSERT_OK(le_sms_IsStatusReportEnabled(&statusReportEnabled));
    LE_ASSERT(true == statusReportEnabled);

     // Send a new message
    LE_INFO("Send a SMS");
    LE_ASSERT(myMsg = le_sms_Create());
    LE_ASSERT_OK(le_sms_SetDestination(myMsg, DestinationNumber));
    LE_ASSERT_OK(le_sms_SetText(myMsg, "Send a SMS Status Report please!"));
    LE_ASSERT_OK(le_sms_Send(myMsg));
    LE_ASSERT_OK(le_sms_GetTpMr(myMsg, &txMessageReference));
    LE_INFO("Message sent with reference: %d", txMessageReference);
    le_sms_Delete(myMsg);

    // Check that message reference has been correctly incremented
    LE_ASSERT(txMessageReference == (oldMessageReference + 1));

    // Wait to check that a Status Report is received
    LE_INFO("Check that a SMS Status Report is received");
    WaitForSem(SyncSemRef, SEMAPHORE_TIMEOUT, LE_OK);

    // Display SMS Status Report data
    LE_ASSERT(LE_SMS_TYPE_STATUS_REPORT == le_sms_GetType(ReceivedSmsRef));
    LE_ASSERT_OK(le_sms_GetTpMr(ReceivedSmsRef, &rxMessageReference));
    LE_INFO("Message reference: %d", rxMessageReference);
    LE_ASSERT_OK(le_sms_GetTpRa(ReceivedSmsRef, &tora, ra, sizeof(ra)));
    LE_INFO("Recipient Address: %s (Type of Address %d)", ra, tora);
    LE_ASSERT_OK(le_sms_GetTpScTs(ReceivedSmsRef, timestamp, sizeof(timestamp)));
    LE_INFO("Service Centre Time Stamp: %s", timestamp);
    LE_ASSERT_OK(le_sms_GetTpDt(ReceivedSmsRef, timestamp, sizeof(timestamp)));
    LE_INFO("Discharge Time: %s", timestamp);
    LE_ASSERT_OK(le_sms_GetTpSt(ReceivedSmsRef, &status));
    LE_INFO("Status: %d", status);

    // Check that the message reference of Status Report is the same as the message previously sent
    LE_ASSERT(rxMessageReference == txMessageReference)

    // Clean up
    le_sem_Delete(SyncSemRef);
    le_thread_Cancel(rxThread);

    LE_INFO("=== End of SMS Status Report test ===");
    exit(EXIT_SUCCESS);
}
