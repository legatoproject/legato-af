/**
 * @file main.c
 *
 * This module implements the unit tests for the Remote SIM service API.
 *
 * API tested:
 * - le_rsim_AddMessageHandler
 * - le_rsim_SendMessage
 * - le_rsim_RemoveMessageHandler
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_rsim_local.h"
#include "pa_rsim.h"
#include "pa_rsim_simu.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Short semaphore timeout in seconds
 */
//--------------------------------------------------------------------------------------------------
#define SHORT_TIMEOUT   1

//--------------------------------------------------------------------------------------------------
/**
 *  Long semaphore timeout in seconds
 */
//--------------------------------------------------------------------------------------------------
#define LONG_TIMEOUT    5

//--------------------------------------------------------------------------------------------------
/**
 *  Semaphore used to synchronize the test
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t AppSemaphore;

//--------------------------------------------------------------------------------------------------
/**
 *  Application handler thread reference
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t  AppHandlerThreadRef;

//--------------------------------------------------------------------------------------------------
/**
 *  Application message handler reference
 */
//--------------------------------------------------------------------------------------------------
static le_rsim_MessageHandlerRef_t  AppMessageHandlerRef;

//--------------------------------------------------------------------------------------------------
/**
 *  Expected message
 */
//--------------------------------------------------------------------------------------------------
static uint8_t ExpectedMessage[LE_RSIM_MAX_MSG_SIZE] = {0};

//--------------------------------------------------------------------------------------------------
/**
 *  Expected message size
 */
//--------------------------------------------------------------------------------------------------
static size_t ExpectedMessageSize = 0;

//--------------------------------------------------------------------------------------------------
/**
 *  Expected SAP messages
 */
//--------------------------------------------------------------------------------------------------
static uint8_t ConnectReq1[12] =
{
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x14, 0x00, 0x00
};
static size_t ConnectReq1Length = 12;
static uint8_t ConnectReq2[12] =
{
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xFA, 0x00, 0x00
};
static size_t ConnectReq2Length = 12;
static uint8_t TransferAtrReq[4] =
{
    0x07, 0x00, 0x00, 0x00
};
static size_t TransferAtrReqLength = 4;
static uint8_t TransferApduReq[16] =
{
    0x05, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x07, 0x00, 0xA4, 0x00, 0x04, 0x02, 0x6F, 0xB7, 0x00
};
static size_t TransferApduReqLength = 16;
static uint8_t PowerSimOffReq[4] =
{
    0x09, 0x00, 0x00, 0x00
};
static size_t PowerSimOffReqLength = 4;
static uint8_t PowerSimOnReq[4] =
{
    0x0B, 0x00, 0x00, 0x00
};
static size_t PowerSimOnReqLength = 4;
static uint8_t ResetSimReq[4] =
{
    0x0D, 0x00, 0x00, 0x00
};
static size_t ResetSimReqLength = 4;
static uint8_t DisconnectReq[4] =
{
    0x02, 0x00, 0x00, 0x00
};
static size_t DisconnectReqLength = 4;

//--------------------------------------------------------------------------------------------------
/**
 *  ATR and APDU
 */
//--------------------------------------------------------------------------------------------------
static uint8_t AtrData[23] =
{
    0x3B, 0x9F, 0x96, 0x80, 0x3F, 0xC7, 0xA0, 0x80, 0x31, 0xE0, 0x73, 0xFE, 0x21, 0x1B, 0x64, 0x07,
    0x68, 0x9A, 0x00, 0x82, 0x90, 0x00, 0xB4
};
static size_t AtrLength = 23;
static uint8_t Apdu1Data[7] =
{
    0x00, 0xA4, 0x00, 0x04, 0x02, 0x6F, 0xB7
};
static size_t Apdu1Length = 7;
static uint8_t Apdu2Data[2] =
{
    0x90, 0x00
};
static size_t Apdu2Length = 2;


//--------------------------------------------------------------------------------------------------
// Utility functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Synchronize test thread (i.e. main) and application thread
 */
//--------------------------------------------------------------------------------------------------
static void SynchronizeTest
(
    void
)
{
    le_clk_Time_t timeToWait = {LONG_TIMEOUT, 0};

    LE_ASSERT_OK(le_sem_WaitWithTimeOut(AppSemaphore, timeToWait));
}

// -------------------------------------------------------------------------------------------------
/**
 *  Callback for RSIM messages sending result
 */
// -------------------------------------------------------------------------------------------------
static void CallbackHandler
(
    uint8_t messageId,
    le_result_t result,
    void* contextPtr
)
{
    LE_DEBUG("Sending result: messageId=%d, result=%d, context=%p", messageId, result, contextPtr);

    // Check sending result against expected result
    LE_ASSERT(result == (le_result_t)contextPtr)

    le_sem_Post(AppSemaphore);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Event callback for RSIM messages notification
 */
// -------------------------------------------------------------------------------------------------
static void MessageHandler
(
    const uint8_t* messagePtr,
    size_t messageNumElements,
    void* contextPtr
)
{
    int i = 0;

    LE_DEBUG("Received a RSIM message:");
    LE_DUMP(messagePtr, messageNumElements);
    LE_DEBUG("Expected RSIM message:");
    LE_DUMP(ExpectedMessage, ExpectedMessageSize);

    LE_ASSERT(messageNumElements == ExpectedMessageSize);
    for (i = 0; i < messageNumElements; i++)
    {
        LE_ASSERT(messagePtr[i] == ExpectedMessage[i]);
    }

    le_sem_Post(AppSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Thread used to register handler and receive the RSIM message notifications
 */
//--------------------------------------------------------------------------------------------------
static void* AppHandler
(
    void* ctxPtr
)
{
    // Register handler for RSIM messages notification
    AppMessageHandlerRef = le_rsim_AddMessageHandler(MessageHandler, NULL);
    LE_ASSERT(NULL != AppMessageHandlerRef);
    LE_INFO("MessageHandler %p added", AppMessageHandlerRef);

    // Add a second handler, which should be rejected
    LE_ASSERT(NULL == le_rsim_AddMessageHandler(MessageHandler, NULL));

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(AppSemaphore);

    // Run the event loop
    le_event_RunLoop();

    return NULL;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Remove RSIM handler
 */
// -------------------------------------------------------------------------------------------------
static void AppRemoveHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    // Unregister message handler
    LE_INFO("Unregister MessageHandler %p", AppMessageHandlerRef);
    le_rsim_RemoveMessageHandler(AppMessageHandlerRef);

    le_sem_Post(AppSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send SAP CONNECT_RESP message
 */
//--------------------------------------------------------------------------------------------------
static void SendSapConnectResp
(
    uint8_t  connectionStatus,
    uint16_t maxMsgSize
)
{
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];
    size_t  messageSize = 0;
    memset(&message, 0, sizeof(message));

    // ConnectionStatus 'Error, Server does not support maximum message size'
    if (SAP_CONNSTATUS_MAXMSGSIZE_NOK == connectionStatus)
    {
        // SAP header
        message[0]  = SAP_MSGID_CONNECT_RESP;               // MsgId (CONNECT_RESP)
        message[1]  = 0x02;                                 // Parameters number
        message[2]  = 0x00;                                 // Reserved
        message[3]  = 0x00;                                 // Reserved

        // Parameter header
        message[4]  = SAP_PARAMID_CONNECTION_STATUS;        // Parameter Id
        message[5]  = 0x00;                                 // Reserved
        message[6]  = 0x00;                                 // Parameter length (MSB)
        message[7]  = SAP_LENGTH_CONNECTION_STATUS;         // Parameter length (LSB)

        // Parameter value (ConnectionStatus)
        message[8]  = connectionStatus;                     // ConnectionStatus
        message[9]  = 0x00;                                 // Padding
        message[10] = 0x00;                                 // Padding
        message[11] = 0x00;                                 // Padding

        // Parameter header
        message[12] = SAP_PARAMID_MAX_MSG_SIZE;             // Parameter Id
        message[13] = 0x00;                                 // Reserved
        message[14] = 0x00;                                 // Parameter length (MSB)
        message[15] = SAP_LENGTH_MAX_MSG_SIZE;              // Parameter length (LSB)

        // Parameter value (MaxMsgSize)
        message[16] = ((maxMsgSize & 0xFF00U) >> MSB_SHIFT);// MaxMsgSize (MSB)
        message[17] = (maxMsgSize & 0x00FF);                // MaxMsgSize (LSB)
        message[18] = 0x00;                                 // Padding
        message[19] = 0x00;                                 // Padding

        // Message size
        messageSize = 20;
    }
    else
    {
        // SAP header
        message[0]  = SAP_MSGID_CONNECT_RESP;           // MsgId (CONNECT_RESP)
        message[1]  = 0x01;                             // Parameters number
        message[2]  = 0x00;                             // Reserved
        message[3]  = 0x00;                             // Reserved

        // Parameter header
        message[4]  = SAP_PARAMID_CONNECTION_STATUS;    // Parameter Id
        message[5]  = 0x00;                             // Reserved
        message[6]  = 0x00;                             // Parameter length (MSB)
        message[7]  = SAP_LENGTH_CONNECTION_STATUS;     // Parameter length (LSB)

        // Parameter value (ConnectionStatus)
        message[8]  = connectionStatus;                 // ConnectionStatus
        message[9]  = 0x00;                             // Padding
        message[10] = 0x00;                             // Padding
        message[11] = 0x00;                             // Padding

        // Message size
        messageSize = 12;
    }

    // Send message
    LE_DEBUG("Send CONNECT_RESP message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message, messageSize, CallbackHandler, (void*)LE_OK));
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send SAP STATUS_IND message
 */
//--------------------------------------------------------------------------------------------------
static void SendSapStatusInd
(
    uint8_t statusChange
)
{
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];
    size_t  messageSize = 0;
    memset(&message, 0, sizeof(message));

    // SAP header
    message[0]  = SAP_MSGID_STATUS_IND;         // MsgId (STATUS_IND)
    message[1]  = 0x01;                         // Parameters number
    message[2]  = 0x00;                         // Reserved
    message[3]  = 0x00;                         // Reserved

    // Parameter header
    message[4]  = SAP_PARAMID_STATUS_CHANGE;    // Parameter Id
    message[5]  = 0x00;                         // Reserved
    message[6]  = 0x00;                         // Parameter length (MSB)
    message[7]  = SAP_LENGTH_STATUS_CHANGE;     // Parameter length (LSB)

    // Parameter value (StatusChange)
    message[8]  = statusChange;                 // StatusChange
    message[9]  = 0x00;                         // Padding
    message[10] = 0x00;                         // Padding
    message[11] = 0x00;                         // Padding

    // Message size
    messageSize = 12;

    // Send message
    LE_DEBUG("Send STATUS_IND message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message, messageSize, CallbackHandler, (void*)LE_OK));
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send SAP TRANSFER_ATR_RESP message
 */
//--------------------------------------------------------------------------------------------------
static void SendSapTransferAtrResp
(
    uint8_t  resultCode,
    uint8_t* atrPtr,
    uint16_t atrLen
)
{
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];
    size_t  messageSize = 0;
    memset(&message, 0, sizeof(message));

    // ResultCode 'OK, request processed correctly'
    if (SAP_RESULTCODE_OK == resultCode)
    {
        // SAP header
        message[0]  = SAP_MSGID_TRANSFER_ATR_RESP;      // MsgId (TRANSFER_ATR_RESP)
        message[1]  = 0x02;                             // Parameters number
        message[2]  = 0x00;                             // Reserved
        message[3]  = 0x00;                             // Reserved

        // Parameter header
        message[4]  = SAP_PARAMID_RESULT_CODE;          // Parameter Id
        message[5]  = 0x00;                             // Reserved
        message[6]  = 0x00;                             // Parameter length (MSB)
        message[7]  = SAP_LENGTH_RESULT_CODE;           // Parameter length (LSB)

        // Parameter value (ResultCode)
        message[8]  = resultCode;                       // ResultCode
        message[9]  = 0x00;                             // Padding
        message[10] = 0x00;                             // Padding
        message[11] = 0x00;                             // Padding

        // Parameter header
        message[12] = SAP_PARAMID_ATR;                  // Parameter Id
        message[13] = 0x00;                             // Reserved
        message[14] = ((atrLen & 0xFF00U) >> MSB_SHIFT);// Parameter length (MSB)
        message[15] = (atrLen & 0x00FF);                // Parameter length (LSB)

        // Current message size
        messageSize = 16;

        // Parameter value (ATR)
        memcpy(&message[messageSize], atrPtr, atrLen);
        messageSize += atrLen;

        // Message size should be 4-byte aligned
        messageSize += (4 - (messageSize % 4));
    }
    else
    {
        // SAP header
        message[0]  = SAP_MSGID_TRANSFER_ATR_RESP;  // MsgId (TRANSFER_ATR_RESP)
        message[1]  = 0x01;                         // Parameters number
        message[2]  = 0x00;                         // Reserved
        message[3]  = 0x00;                         // Reserved

        // Parameter header
        message[4]  = SAP_PARAMID_RESULT_CODE;      // Parameter Id
        message[5]  = 0x00;                         // Reserved
        message[6]  = 0x00;                         // Parameter length (MSB)
        message[7]  = SAP_LENGTH_RESULT_CODE;       // Parameter length (LSB)

        // Parameter value (ResultCode)
        message[8]  = resultCode;                   // ResultCode
        message[9]  = 0x00;                         // Padding
        message[10] = 0x00;                         // Padding
        message[11] = 0x00;                         // Padding

        // Message size
        messageSize = 12;
    }

    // Send message
    LE_DEBUG("Send TRANSFER_ATR_RESP message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message, messageSize, CallbackHandler, (void*)LE_OK));
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send SAP TRANSFER_APDU_RESP message
 */
//--------------------------------------------------------------------------------------------------
static void SendSapTransferApduResp
(
    uint8_t  resultCode,
    uint8_t* apduPtr,
    uint16_t apduLen
)
{
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];
    size_t  messageSize = 0;
    memset(&message, 0, sizeof(message));

    // ResultCode 'OK, request processed correctly'
    if (SAP_RESULTCODE_OK == resultCode)
    {
        // SAP header
        message[0]  = SAP_MSGID_TRANSFER_APDU_RESP;         // MsgId (TRANSFER_APDU_RESP)
        message[1]  = 0x02;                                 // Parameters number
        message[2]  = 0x00;                                 // Reserved
        message[3]  = 0x00;                                 // Reserved

        // Parameter header
        message[4]  = SAP_PARAMID_RESULT_CODE;              // Parameter Id
        message[5]  = 0x00;                                 // Reserved
        message[6]  = 0x00;                                 // Parameter length (MSB)
        message[7]  = SAP_LENGTH_RESULT_CODE;               // Parameter length (LSB)

        // Parameter value (ResultCode)
        message[8]  = resultCode;                           // ResultCode
        message[9]  = 0x00;                                 // Padding
        message[10] = 0x00;                                 // Padding
        message[11] = 0x00;                                 // Padding

        // Parameter header
        message[12] = SAP_PARAMID_COMMAND_APDU;             // Parameter Id
        message[13] = 0x00;                                 // Reserved
        message[14] = ((apduLen & 0xFF00U) >> MSB_SHIFT);   // Parameter length (MSB)
        message[15] = (apduLen & 0x00FF);                   // Parameter length (LSB)

        // Current message size
        messageSize = 16;

        // Parameter value (APDU)
        memcpy(&message[messageSize], apduPtr, apduLen);
        messageSize += apduLen;

        // Message size should be 4-byte aligned
        messageSize += (4 - (messageSize % 4));
    }
    else
    {
        // SAP header
        message[0]  = SAP_MSGID_TRANSFER_APDU_RESP;     // MsgId (TRANSFER_APDU_RESP)
        message[1]  = 0x01;                             // Parameters number
        message[2]  = 0x00;                             // Reserved
        message[3]  = 0x00;                             // Reserved

        // Parameter header
        message[4]  = SAP_PARAMID_RESULT_CODE;          // Parameter Id
        message[5]  = 0x00;                             // Reserved
        message[6]  = 0x00;                             // Parameter length (MSB)
        message[7]  = SAP_LENGTH_RESULT_CODE;           // Parameter length (LSB)

        // Parameter value (ResultCode)
        message[8]  = resultCode;                       // ResultCode
        message[9]  = 0x00;                             // Padding
        message[10] = 0x00;                             // Padding
        message[11] = 0x00;                             // Padding

        // Message size
        messageSize = 12;
    }

    // Send message
    LE_DEBUG("Send TRANSFER_APDU_RESP message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message, messageSize, CallbackHandler, (void*)LE_OK));
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send SAP POWER_SIM_OFF_RESP message
 */
//--------------------------------------------------------------------------------------------------
static void SendSapPowerSimOffResp
(
    uint8_t resultCode
)
{
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];
    size_t  messageSize = 0;
    memset(&message, 0, sizeof(message));

    // SAP header
    message[0]  = SAP_MSGID_POWER_SIM_OFF_RESP;     // MsgId (POWER_SIM_OFF_RESP)
    message[1]  = 0x01;                             // Parameters number
    message[2]  = 0x00;                             // Reserved
    message[3]  = 0x00;                             // Reserved

    // Parameter header
    message[4]  = SAP_PARAMID_RESULT_CODE;          // Parameter Id
    message[5]  = 0x00;                             // Reserved
    message[6]  = 0x00;                             // Parameter length (MSB)
    message[7]  = SAP_LENGTH_RESULT_CODE;           // Parameter length (LSB)

    // Parameter value (ResultCode)
    message[8]  = resultCode;                       // ResultCode
    message[9]  = 0x00;                             // Padding
    message[10] = 0x00;                             // Padding
    message[11] = 0x00;                             // Padding

    // Message size
    messageSize = 12;

    // Send message
    LE_DEBUG("Send POWER_SIM_OFF_RESP message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message, messageSize, CallbackHandler, (void*)LE_OK));
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send SAP POWER_SIM_ON_RESP message
 */
//--------------------------------------------------------------------------------------------------
static void SendSapPowerSimOnResp
(
    uint8_t resultCode
)
{
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];
    size_t  messageSize = 0;
    memset(&message, 0, sizeof(message));

    // SAP header
    message[0]  = SAP_MSGID_POWER_SIM_ON_RESP;  // MsgId (POWER_SIM_ON_RESP)
    message[1]  = 0x01;                         // Parameters number
    message[2]  = 0x00;                         // Reserved
    message[3]  = 0x00;                         // Reserved

    // Parameter header
    message[4]  = SAP_PARAMID_RESULT_CODE;      // Parameter Id
    message[5]  = 0x00;                         // Reserved
    message[6]  = 0x00;                         // Parameter length (MSB)
    message[7]  = SAP_LENGTH_RESULT_CODE;       // Parameter length (LSB)

    // Parameter value (ResultCode)
    message[8]  = resultCode;                   // ResultCode
    message[9]  = 0x00;                         // Padding
    message[10] = 0x00;                         // Padding
    message[11] = 0x00;                         // Padding

    // Message size
    messageSize = 12;

    // Send message
    LE_DEBUG("Send POWER_SIM_ON_RESP message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message, messageSize, CallbackHandler, (void*)LE_OK));
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send SAP RESET_SIM_RESP message
 */
//--------------------------------------------------------------------------------------------------
static void SendSapResetSimResp
(
    uint8_t resultCode
)
{
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];
    size_t  messageSize = 0;
    memset(&message, 0, sizeof(message));

    // SAP header
    message[0]  = SAP_MSGID_RESET_SIM_RESP;     // MsgId (RESET_SIM_RESP)
    message[1]  = 0x01;                         // Parameters number
    message[2]  = 0x00;                         // Reserved
    message[3]  = 0x00;                         // Reserved

    // Parameter header
    message[4]  = SAP_PARAMID_RESULT_CODE;      // Parameter Id
    message[5]  = 0x00;                         // Reserved
    message[6]  = 0x00;                         // Parameter length (MSB)
    message[7]  = SAP_LENGTH_RESULT_CODE;       // Parameter length (LSB)

    // Parameter value
    message[8]  = resultCode;                   // ResultCode
    message[9]  = 0x00;                         // Padding
    message[10] = 0x00;                         // Padding
    message[11] = 0x00;                         // Padding

    // Message size
    messageSize = 12;

    // Send message
    LE_DEBUG("Send RESET_SIM_RESP message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message, messageSize, CallbackHandler, (void*)LE_OK));
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send SAP DISCONNECT_IND message
 */
//--------------------------------------------------------------------------------------------------
static void SendSapDisconnectInd
(
    uint8_t disconnectionType
)
{
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];
    size_t  messageSize = 0;
    memset(&message, 0, sizeof(message));

    // SAP header
    message[0]  = SAP_MSGID_DISCONNECT_IND;         // MsgId (DISCONNECT_IND)
    message[1]  = 0x01;                             // Parameters number
    message[2]  = 0x00;                             // Reserved
    message[3]  = 0x00;                             // Reserved

    // Parameter header
    message[4]  = SAP_PARAMID_DISCONNECTION_TYPE;   // Parameter Id
    message[5]  = 0x00;                             // Reserved
    message[6]  = 0x00;                             // Parameter length (MSB)
    message[7]  = SAP_LENGTH_DISCONNECTION_TYPE;    // Parameter length (LSB)

    // Parameter value (DisconnectionType)
    message[8]  = disconnectionType;                // DisconnectionType
    message[9]  = 0x00;                             // Padding
    message[10] = 0x00;                             // Padding
    message[11] = 0x00;                             // Padding

    // Message size
    messageSize = 12;

    // Send message
    LE_DEBUG("Send DISCONNECT_IND message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message, messageSize, CallbackHandler, (void*)LE_OK));
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send SAP DISCONNECT_RESP message
 */
//--------------------------------------------------------------------------------------------------
static void SendSapDisconnectResp
(
    void
)
{
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];
    size_t  messageSize = 0;
    memset(&message, 0, sizeof(message));

    // SAP header
    message[0] = SAP_MSGID_DISCONNECT_RESP;     // MsgId (DISCONNECT_RESP)
    message[1] = 0x00;                          // Parameters number
    message[2] = 0x00;                          // Reserved
    message[3] = 0x00;                          // Reserved

    // Message size
    messageSize = 4;

    // Send message
    LE_DEBUG("Send DISCONNECT_RESP message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message, messageSize, CallbackHandler, (void*)LE_OK));
}


//--------------------------------------------------------------------------------------------------
// Test functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Add handler to receive RSIM messages
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void TestRsimAddHandler
(
    void
)
{
    // Start a task to receive the Remote SIM service messages
    AppHandlerThreadRef = le_thread_Create("AppThread", AppHandler, NULL);
    le_thread_Start(AppHandlerThreadRef);

    // Wait for the task initialization before continuing the test
    SynchronizeTest();
}

//--------------------------------------------------------------------------------------------------
/**
 * Establish the SAP connection with the remote SIM service
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void TestRsimConnection
(
    void
)
{
    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, ConnectReq1, ConnectReq1Length);
    ExpectedMessageSize = ConnectReq1Length;

    // Simulate a connection request by the modem
    pa_rsimSimu_SendSimActionRequest(PA_RSIM_CONNECTION);

    // Wait for a CONNECT_REQ message
    SynchronizeTest();

    // No CONNECT_RESP sent, connection establishment timeout:
    // wait for a new CONNECT_REQ message
    SynchronizeTest();

    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, ConnectReq2, ConnectReq2Length);
    ExpectedMessageSize = ConnectReq2Length;

    // Send a CONNECT_RESP message with 'Error, Server does not support maximum message size'
    // and a new maximal message size
    uint16_t serverMaxMsgSize = 250;
    SendSapConnectResp(SAP_CONNSTATUS_MAXMSGSIZE_NOK, serverMaxMsgSize);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for a new CONNECT_REQ message
    SynchronizeTest();

    // Send a CONNECT_RESP message with 'OK, Server can fulfill requirements'
    SendSapConnectResp(SAP_CONNSTATUS_OK, 0);
    // Wait for message sending result
    SynchronizeTest();

    // Send a message which is too long
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];
    size_t  messageSize = serverMaxMsgSize + 1;
    memset(&message, 0, sizeof(message));
    // Send message
    LE_ASSERT(LE_BAD_PARAMETER == le_rsim_SendMessage(message, messageSize, NULL, NULL));

    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, TransferAtrReq, TransferAtrReqLength);
    ExpectedMessageSize = TransferAtrReqLength;

    // Send a STATUS_IND message with 'Card reset'
    SendSapStatusInd(SAP_STATUSCHANGE_CARD_RESET);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for a TRANSFER_ATR_REQ message
    SynchronizeTest();

    // Send a TRANSFER_ATR_RESP message with 'OK, request processed correctly'
    SendSapTransferAtrResp(SAP_RESULTCODE_OK, AtrData, AtrLength);
    // Wait for message sending result
    SynchronizeTest();
}

//--------------------------------------------------------------------------------------------------
/**
 * Exchange APDUs after remote SIM service connection
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void TestRsimApdu
(
    void
)
{
    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, TransferApduReq, TransferApduReqLength);
    ExpectedMessageSize = TransferApduReqLength;

    // Simulate an APDU indication by the modem
    pa_rsimSimu_SendApduInd(Apdu1Data, Apdu1Length);

    // Wait for a TRANSFER_APDU_REQ message
    SynchronizeTest();

    // Send a TRANSFER_APDU_RESP message with 'Error, no reason defined'
    SendSapTransferApduResp(SAP_RESULTCODE_ERROR_NO_REASON, NULL, 0);
    // Wait for message sending result
    SynchronizeTest();

    // Simulate a new APDU indication by the modem
    pa_rsimSimu_SendApduInd(Apdu1Data, Apdu1Length);

    // Wait for a TRANSFER_APDU_REQ message
    SynchronizeTest();

    // Send a TRANSFER_APDU_RESP message with 'OK, request processed correctly'
    SendSapTransferApduResp(SAP_RESULTCODE_OK, Apdu2Data, Apdu2Length);
    // Wait for message sending result
    SynchronizeTest();
}

//--------------------------------------------------------------------------------------------------
/**
 * Remote SIM card reset while remote SIM service is connected
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void TestRsimCardReset
(
    void
)
{
    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, TransferAtrReq, TransferAtrReqLength);
    ExpectedMessageSize = TransferAtrReqLength;

    // Send a STATUS_IND message with 'Card reset'
    SendSapStatusInd(SAP_STATUSCHANGE_CARD_RESET);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for a TRANSFER_ATR_REQ message
    SynchronizeTest();

    // Send a TRANSFER_ATR_RESP message with 'OK, request processed correctly'
    SendSapTransferAtrResp(SAP_RESULTCODE_OK, AtrData, AtrLength);
    // Wait for message sending result
    SynchronizeTest();
}

//--------------------------------------------------------------------------------------------------
/**
 * Remote SIM card power off and on
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void TestRsimPowerOffOn
(
    void
)
{
    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, PowerSimOffReq, PowerSimOffReqLength);
    ExpectedMessageSize = PowerSimOffReqLength;

    // Simulate a remote SIM power off request
    pa_rsimSimu_SendSimActionRequest(PA_RSIM_POWER_DOWN);

    // Wait for a POWER_SIM_OFF_REQ message
    SynchronizeTest();

    // Send a POWER_SIM_OFF_RESP message with 'OK, request processed correctly'
    SendSapPowerSimOffResp(SAP_RESULTCODE_OK);
    // Wait for message sending result
    SynchronizeTest();

    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, PowerSimOnReq, PowerSimOnReqLength);
    ExpectedMessageSize = PowerSimOnReqLength;

    // Simulate a remote SIM power on request
    pa_rsimSimu_SendSimActionRequest(PA_RSIM_POWER_UP);

    // Wait for a POWER_SIM_ON_REQ message
    SynchronizeTest();

    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, TransferAtrReq, TransferAtrReqLength);
    ExpectedMessageSize = TransferAtrReqLength;

    // Send a POWER_SIM_ON_RESP message with 'OK, request processed correctly'
    SendSapPowerSimOnResp(SAP_RESULTCODE_OK);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for a TRANSFER_ATR_REQ message
    SynchronizeTest();

    // Send a TRANSFER_ATR_RESP message with 'OK, request processed correctly'
    SendSapTransferAtrResp(SAP_RESULTCODE_OK, AtrData, AtrLength);
    // Wait for message sending result
    SynchronizeTest();
}

//--------------------------------------------------------------------------------------------------
/**
 * Remote SIM card hot swap
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void TestRsimHotSwap
(
    void
)
{
    // Send a STATUS_IND message with 'Card removed'
    SendSapStatusInd(SAP_STATUSCHANGE_CARD_REMOVED);
    // Wait for message sending result
    SynchronizeTest();

    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, TransferAtrReq, TransferAtrReqLength);
    ExpectedMessageSize = TransferAtrReqLength;

    // Send a STATUS_IND message with 'Card inserted'
    SendSapStatusInd(SAP_STATUSCHANGE_CARD_INSERTED);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for a TRANSFER_ATR_REQ message
    SynchronizeTest();

    // Send a TRANSFER_ATR_RESP message with 'OK, request processed correctly'
    SendSapTransferAtrResp(SAP_RESULTCODE_OK, AtrData, AtrLength);
    // Wait for message sending result
    SynchronizeTest();

    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, ResetSimReq, ResetSimReqLength);
    ExpectedMessageSize = ResetSimReqLength;

    // Simulate a remote SIM reset request
    pa_rsimSimu_SendSimActionRequest(PA_RSIM_RESET);

    // Wait for a RESET_SIM_REQ message
    SynchronizeTest();

    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, TransferAtrReq, TransferAtrReqLength);
    ExpectedMessageSize = TransferAtrReqLength;

    // Send a RESET_SIM_RESP message with 'OK, request processed correctly'
    SendSapResetSimResp(SAP_RESULTCODE_OK);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for a TRANSFER_ATR_REQ message
    SynchronizeTest();

    // Send a TRANSFER_ATR_RESP message with 'OK, request processed correctly'
    SendSapTransferAtrResp(SAP_RESULTCODE_OK, AtrData, AtrLength);
    // Wait for message sending result
    SynchronizeTest();
}

//--------------------------------------------------------------------------------------------------
/**
 * Remote SIM messages with wrong format
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void TestRsimFormatError
(
    void
)
{
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];
    size_t  messageSize = 0;
    memset(&message, 0, sizeof(message));

    // Create wrongly formatted STATUS_IND message
    // 1. Message is too short
    message[0]  = SAP_MSGID_STATUS_IND;         // MsgId (STATUS_IND)
    message[1]  = 0x01;                         // Parameters number
    messageSize = 4;
    // Send message
    LE_DEBUG("Send STATUS_IND message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message,
                                     messageSize,
                                     CallbackHandler,
                                     (void*)LE_FORMAT_ERROR));
    // Wait for message sending result
    SynchronizeTest();

    // 2. Too few parameters
    message[1]  = 0x00;                         // Parameters number
    messageSize = 12;
    // Send message
    LE_DEBUG("Send STATUS_IND message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message,
                                     messageSize,
                                     CallbackHandler,
                                     (void*)LE_FORMAT_ERROR));
    // Wait for message sending result
    SynchronizeTest();

    // 3. Wrong parameter identifier
    message[1]  = 0x01;                         // Parameters number
    message[4]  = SAP_PARAMID_ATR;              // Parameter Id
    // Send message
    LE_DEBUG("Send STATUS_IND message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message,
                                     messageSize,
                                     CallbackHandler,
                                     (void*)LE_FORMAT_ERROR));
    // Wait for message sending result
    SynchronizeTest();

    // 4. Wrong parameter length
    message[4]  = SAP_PARAMID_STATUS_CHANGE;    // Parameter Id
    message[7]  = SAP_LENGTH_MAX_MSG_SIZE;      // Parameter length (LSB)
    // Send message
    LE_DEBUG("Send STATUS_IND message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message,
                                     messageSize,
                                     CallbackHandler,
                                     (void*)LE_FORMAT_ERROR));
    // Wait for message sending result
    SynchronizeTest();

    // 5. Wrong StatuChange value
    message[7]  = SAP_LENGTH_STATUS_CHANGE;             // Parameter length (LSB)
    message[8]  = SAP_STATUSCHANGE_CARD_RECOVERED + 1;  // StatusChange
    // Send message
    LE_DEBUG("Send STATUS_IND message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message,
                                     messageSize,
                                     CallbackHandler,
                                     (void*)LE_FAULT));
    // Wait for message sending result
    SynchronizeTest();
}

//--------------------------------------------------------------------------------------------------
/**
 * Remote SIM service disconnection
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void TestRsimDisconnection
(
    void
)
{
    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, DisconnectReq, DisconnectReqLength);
    ExpectedMessageSize = DisconnectReqLength;

    // Send a DISCONNECT_IND message with 'Graceful'
    SendSapDisconnectInd(SAP_DISCONNTYPE_GRACEFUL);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for a DISCONNECT_REQ message
    SynchronizeTest();

    // Send a DISCONNECT_RESP message
    SendSapDisconnectResp();
    // Wait for message sending result
    SynchronizeTest();

    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, ConnectReq1, ConnectReq1Length);
    ExpectedMessageSize = ConnectReq1Length;

    // Reconnect RSIM service to test DISCONNECT_IND with 'Immediate'
    // Simulate a connection request by the modem
    pa_rsimSimu_SendSimActionRequest(PA_RSIM_CONNECTION);

    // Wait for a CONNECT_REQ message
    SynchronizeTest();

    // Send a CONNECT_RESP message with 'OK, Server can fulfill requirements'
    SendSapConnectResp(SAP_RESULTCODE_OK, 0);
    // Wait for message sending result
    SynchronizeTest();

    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, TransferAtrReq, TransferAtrReqLength);
    ExpectedMessageSize = TransferAtrReqLength;

    // Send a STATUS_IND message with 'Card reset'
    SendSapStatusInd(SAP_STATUSCHANGE_CARD_RESET);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for a TRANSFER_ATR_REQ message
    SynchronizeTest();

    // Send a TRANSFER_ATR_RESP message with 'OK, request processed correctly'
    SendSapTransferAtrResp(SAP_RESULTCODE_OK, AtrData, AtrLength);
    // Wait for message sending result
    SynchronizeTest();

    // Send a DISCONNECT_IND message with 'Immediate'
    SendSapDisconnectInd(SAP_DISCONNTYPE_IMMEDIATE);
    // Wait for message sending result
    SynchronizeTest();

    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, ConnectReq1, ConnectReq1Length);
    ExpectedMessageSize = ConnectReq1Length;

    // Reconnect RSIM service to test disconnection by the modem
    // Simulate a connection request by the modem
    pa_rsimSimu_SendSimActionRequest(PA_RSIM_CONNECTION);

    // Wait for a CONNECT_REQ message
    SynchronizeTest();

    // Send a CONNECT_RESP message with 'OK, ongoing call'
    SendSapConnectResp(SAP_CONNSTATUS_OK_ONGOING_CALL, 0);
    // Wait for message sending result
    SynchronizeTest();

    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, TransferAtrReq, TransferAtrReqLength);
    ExpectedMessageSize = TransferAtrReqLength;

    // Send a STATUS_IND message with 'Card reset'
    SendSapStatusInd(SAP_STATUSCHANGE_CARD_RESET);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for a TRANSFER_ATR_REQ message
    SynchronizeTest();

    // Send a TRANSFER_ATR_RESP message with 'OK, request processed correctly'
    SendSapTransferAtrResp(SAP_RESULTCODE_OK, AtrData, AtrLength);
    // Wait for message sending result
    SynchronizeTest();

    // Set expected message to receive after next action
    memset(ExpectedMessage, 0, sizeof(ExpectedMessage));
    memcpy(ExpectedMessage, DisconnectReq, DisconnectReqLength);
    ExpectedMessageSize = DisconnectReqLength;

    // Simulate a remote SIM disconnection request
    pa_rsimSimu_SendSimActionRequest(PA_RSIM_DISCONNECTION);

    // Wait for a DISCONNECT_REQ message
    SynchronizeTest();

    // Send a DISCONNECT_RESP message
    SendSapDisconnectResp();
    // Wait for message sending result
    SynchronizeTest();
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate errors (unexpected RSIM events, unsupported SAP messages)
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void TestRsimErrors
(
    void
)
{
    le_clk_Time_t timeToWait = {SHORT_TIMEOUT, 0};

    // Remote SIM service is in disconnected state and should not transmit any message
    // when solicited by the modem

    // Simulate a remote SIM power off request
    pa_rsimSimu_SendSimActionRequest(PA_RSIM_POWER_DOWN);
    // Check that no event is received
    LE_ASSERT(LE_TIMEOUT == le_sem_WaitWithTimeOut(AppSemaphore, timeToWait));

    // Simulate a remote SIM power on request
    pa_rsimSimu_SendSimActionRequest(PA_RSIM_POWER_UP);
    // Check that no event is received
    LE_ASSERT(LE_TIMEOUT == le_sem_WaitWithTimeOut(AppSemaphore, timeToWait));

    // Simulate a remote SIM power on request
    pa_rsimSimu_SendSimActionRequest(PA_RSIM_RESET);
    // Check that no event is received
    LE_ASSERT(LE_TIMEOUT == le_sem_WaitWithTimeOut(AppSemaphore, timeToWait));

    // Simulate a remote SIM disconnection request
    pa_rsimSimu_SendSimActionRequest(PA_RSIM_DISCONNECTION);
    // Check that no event is received
    LE_ASSERT(LE_TIMEOUT == le_sem_WaitWithTimeOut(AppSemaphore, timeToWait));

    // Simulate an APDU indication by the modem
    pa_rsimSimu_SendApduInd(Apdu1Data, Apdu1Length);
    // Check that no event is received
    LE_ASSERT(LE_TIMEOUT == le_sem_WaitWithTimeOut(AppSemaphore, timeToWait));

    // Send unsupported SAP messages
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];
    size_t  messageSize = 0;
    memset(&message, 0, sizeof(message));

    // Create SET_TRANSPORT_PROTOCOL_RESP message
    message[0]  = SAP_MSGID_SET_TRANSPORT_PROTOCOL_RESP;    // MsgId
    messageSize = 4;
    // Send message
    LE_DEBUG("Send SET_TRANSPORT_PROTOCOL_RESP message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message,
                                     messageSize,
                                     CallbackHandler,
                                     (void*)LE_UNSUPPORTED));
    // Wait for message sending result
    SynchronizeTest();

    // Create TRANSFER_CARD_READER_STATUS_RESP message
    message[0]  = SAP_MSGID_TRANSFER_CARD_READER_STATUS_RESP;   // MsgId
    messageSize = 4;
    // Send message
    LE_DEBUG("Send TRANSFER_CARD_READER_STATUS_RESP message:");
    LE_DUMP(message, messageSize);
    LE_ASSERT_OK(le_rsim_SendMessage(message,
                                     messageSize,
                                     CallbackHandler,
                                     (void*)LE_UNSUPPORTED));
    // Wait for message sending result
    SynchronizeTest();
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler and check that no event is received
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void TestRsimRemoveHandler
(
    void
)
{
    // Unregister message handler
    le_event_QueueFunctionToThread(AppHandlerThreadRef, AppRemoveHandler, NULL, NULL);
    SynchronizeTest();

    // Simulate a connection request by the modem
    pa_rsimSimu_SendSimActionRequest(PA_RSIM_CONNECTION);

    // Check that no event is received
    le_clk_Time_t timeToWait = {SHORT_TIMEOUT, 0};
    LE_ASSERT(LE_TIMEOUT == le_sem_WaitWithTimeOut(AppSemaphore, timeToWait));
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread used to launch the unit tests, simulating an application using the remote SIM service
 */
//--------------------------------------------------------------------------------------------------
static void* RemoteSimUnitTestThread
(
    void* contextPtr
)
{
    LE_INFO("======== Start UnitTest of RSIM API ========");

    // Create a semaphore to synchronize the test
    AppSemaphore = le_sem_Create("AppSemaphore", 0);

    LE_INFO("======== Test Add handler ========");
    TestRsimAddHandler();

    LE_INFO("======== Test Connection ========");
    TestRsimConnection();

    LE_INFO("======== Test APDU exchange ========");
    TestRsimApdu();

    LE_INFO("======== Test SIM card reset ========");
    TestRsimCardReset();

    LE_INFO("======== Test SIM card power off/on ========");
    TestRsimPowerOffOn();

    LE_INFO("======== Test SIM card hot swap ========");
    TestRsimHotSwap();

    LE_INFO("======== Test RSIM format errors ========");
    TestRsimFormatError();

    LE_INFO("======== Test Disconnection ========");
    TestRsimDisconnection();

    LE_INFO("======== Test Errors ========");
    TestRsimErrors();

    LE_INFO("======== Test Remove handler ========");
    TestRsimRemoveHandler();

    // Delete semaphore
    le_sem_Delete(AppSemaphore);

    LE_INFO("======== UnitTest of RSIM API ends with SUCCESS ========");

    exit(EXIT_SUCCESS);

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Main of the test
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // To reactivate for all DEBUG logs
//    le_log_SetFilterLevel(LE_LOG_DEBUG);

    // Initialize the simulated PA
    pa_rsim_Init();

    // Initialization of necessary components
    le_rsim_Init();

    // Start the unit test thread simulating an application using the remote SIM service
    le_thread_Start(le_thread_Create("RSIM UT Thread", RemoteSimUnitTestThread, NULL));
}


