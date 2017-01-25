 /**
  * @file rsimTest.c
  *
  * This application implements a basic remote SIM server using the Legato remote SIM service.
  *
  * No physical remote SIM card is connected or supported by this server and all SIM responses
  * are simulated. This application is therefore only used for sanity tests.
  *
  * The remote SIM server does the following:
  * - Register a RSIM message handler
  * - Receive a SAP connection request, establish the connection and send the ATR
  * - Receive a first APDU request and respond with an APDU response error
  * - Receive a second APDU request and respond with a correct APDU response
  * - Receive a third APDU request and trigger a graceful SAP disconnection
  * - Exit
  *
  * @note
  * - Ensure that your platform supports the remote SIM service before using it
  * - Select the remote SIM card and reboot before using it
  * - The application does not start automatically and should be started with "app start rsimTest"
  *
  * <HR>
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool size
 */
//--------------------------------------------------------------------------------------------------
#define RSIM_EVENTS_POOL_SIZE   2

//--------------------------------------------------------------------------------------------------
/**
 * SAP message identifiers (cf. SIM Access Profile specification)
 */
//--------------------------------------------------------------------------------------------------
#define SAP_MSGID_CONNECT_REQ                       0x00
#define SAP_MSGID_CONNECT_RESP                      0x01
#define SAP_MSGID_DISCONNECT_REQ                    0x02
#define SAP_MSGID_DISCONNECT_RESP                   0x03
#define SAP_MSGID_DISCONNECT_IND                    0x04
#define SAP_MSGID_TRANSFER_APDU_REQ                 0x05
#define SAP_MSGID_TRANSFER_APDU_RESP                0x06
#define SAP_MSGID_TRANSFER_ATR_REQ                  0x07
#define SAP_MSGID_TRANSFER_ATR_RESP                 0x08
#define SAP_MSGID_POWER_SIM_OFF_REQ                 0x09
#define SAP_MSGID_POWER_SIM_OFF_RESP                0x0A
#define SAP_MSGID_POWER_SIM_ON_REQ                  0x0B
#define SAP_MSGID_POWER_SIM_ON_RESP                 0x0C
#define SAP_MSGID_RESET_SIM_REQ                     0x0D
#define SAP_MSGID_RESET_SIM_RESP                    0x0E
#define SAP_MSGID_TRANSFER_CARD_READER_STATUS_REQ   0x0F
#define SAP_MSGID_TRANSFER_CARD_READER_STATUS_RESP  0x10
#define SAP_MSGID_STATUS_IND                        0x11
#define SAP_MSGID_ERROR_RESP                        0x12
#define SAP_MSGID_SET_TRANSPORT_PROTOCOL_REQ        0x13
#define SAP_MSGID_SET_TRANSPORT_PROTOCOL_RESP       0x14

//--------------------------------------------------------------------------------------------------
/**
 * SAP parameters identifiers (cf. SIM Access Profile specification)
 */
//--------------------------------------------------------------------------------------------------
#define SAP_PARAMID_MAX_MSG_SIZE            0x00
#define SAP_PARAMID_CONNECTION_STATUS       0x01
#define SAP_PARAMID_RESULT_CODE             0x02
#define SAP_PARAMID_DISCONNECTION_TYPE      0x03
#define SAP_PARAMID_COMMAND_APDU            0x04
#define SAP_PARAMID_COMMAND_APDU_7816       0x10
#define SAP_PARAMID_RESPONSE_APDU           0x05
#define SAP_PARAMID_ATR                     0x06
#define SAP_PARAMID_CARD_READER_STATUS      0x07
#define SAP_PARAMID_STATUS_CHANGE           0x08
#define SAP_PARAMID_TRANSPORT_PROTOCOL      0x09

//--------------------------------------------------------------------------------------------------
/**
 * Length in bytes of different parameters of SAP messages (cf. SIM Access Profile specification).
 * Only parameters with fixed lengths are listed here
 */
//--------------------------------------------------------------------------------------------------
#define SAP_LENGTH_SAP_HEADER           4   ///< 4 bytes header for SAP messages
#define SAP_LENGTH_PARAM_HEADER         4   ///< 4 bytes header for each parameter
#define SAP_LENGTH_MAX_MSG_SIZE         2
#define SAP_LENGTH_CONNECTION_STATUS    1
#define SAP_LENGTH_RESULT_CODE          1
#define SAP_LENGTH_DISCONNECTION_TYPE   1
#define SAP_LENGTH_CARD_READER_STATUS   1
#define SAP_LENGTH_STATUS_CHANGE        1
#define SAP_LENGTH_TRANSPORT_PROTOCOL   1
#define SAP_LENGTH_PARAM_PAYLOAD        4   ///< Parameter payload is 4 bytes long with padding
#define SAP_LENGTH_PARAM                (SAP_LENGTH_PARAM_HEADER + SAP_LENGTH_PARAM_PAYLOAD)

//--------------------------------------------------------------------------------------------------
/**
 * SAP ConnectionStatus values (cf. SIM Access Profile specification section 5.2.2)
 */
//--------------------------------------------------------------------------------------------------
#define SAP_CONNSTATUS_OK               0x00    ///< OK, Server can fulfill requirements
#define SAP_CONNSTATUS_SERVER_NOK       0x01    ///< Error, Server unable to establish connection
#define SAP_CONNSTATUS_MAXMSGSIZE_NOK   0x02    ///< Error, Server does not support
                                                ///< maximum message size
#define SAP_CONNSTATUS_SMALL_MAXMSGSIZE 0x03    ///< Error, maximum message size
                                                ///< by Client is too small
#define SAP_CONNSTATUS_OK_ONGOING_CALL  0x04    ///< OK, ongoing call

//--------------------------------------------------------------------------------------------------
/**
 * SAP DisconnectionType values (cf. SIM Access Profile specification section 5.2.3)
 */
//--------------------------------------------------------------------------------------------------
#define SAP_DISCONNTYPE_GRACEFUL        0x00    ///< Graceful
#define SAP_DISCONNTYPE_IMMEDIATE       0x01    ///< Immediate

//--------------------------------------------------------------------------------------------------
/**
 * SAP ResultCode values (cf. SIM Access Profile specification section 5.2.4)
 */
//--------------------------------------------------------------------------------------------------
#define SAP_RESULTCODE_OK                   0x00    ///< OK, request processed correctly
#define SAP_RESULTCODE_ERROR_NO_REASON      0x01    ///< Error, no reason defined
#define SAP_RESULTCODE_ERROR_CARD_NOK       0x02    ///< Error, card not accessible
#define SAP_RESULTCODE_ERROR_CARD_OFF       0x03    ///< Error, card (already) powered off
#define SAP_RESULTCODE_ERROR_CARD_REMOVED   0x04    ///< Error, card removed
#define SAP_RESULTCODE_ERROR_CARD_ON        0x05    ///< Error, card already powered on
#define SAP_RESULTCODE_ERROR_NO_DATA        0x06    ///< Error, data not available
#define SAP_RESULTCODE_ERROR_NOT_SUPPORTED  0x07    ///< Error, not supported

//--------------------------------------------------------------------------------------------------
/**
 * SAP StatusChange values (cf. SIM Access Profile specification section 5.2.8)
 */
//--------------------------------------------------------------------------------------------------
#define SAP_STATUSCHANGE_UNKNOWN_ERROR  0x00    ///< Unknown Error
#define SAP_STATUSCHANGE_CARD_RESET     0x01    ///< Card reset
#define SAP_STATUSCHANGE_CARD_NOK       0x02    ///< Card not accessible
#define SAP_STATUSCHANGE_CARD_REMOVED   0x03    ///< Card removed
#define SAP_STATUSCHANGE_CARD_INSERTED  0x04    ///< Card inserted
#define SAP_STATUSCHANGE_CARD_RECOVERED 0x05    ///< Card recovered

//--------------------------------------------------------------------------------------------------
/**
 * Bit shift to access MSB byte
 */
//--------------------------------------------------------------------------------------------------
#define MSB_SHIFT   8


//--------------------------------------------------------------------------------------------------
// Data structures
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * RSIM message sending structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];      ///< Message
    size_t  messageSize;                        ///< Message size
    le_rsim_CallbackHandlerFunc_t callbackPtr;  ///< Callback response
    void*   context;                            ///< Associated context
}
RsimMessageSending_t;


//--------------------------------------------------------------------------------------------------
// Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool used to transfer RSIM messages sending to the application thread
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t RsimMessagesPool;

//--------------------------------------------------------------------------------------------------
/**
 *  RSIM message handler reference
 */
//--------------------------------------------------------------------------------------------------
static le_rsim_MessageHandlerRef_t RsimMessageHandlerRef;

//--------------------------------------------------------------------------------------------------
/**
 *  Semaphore used to synchronize the test
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t AppSemaphore;

//--------------------------------------------------------------------------------------------------
/**
 *  Application thread reference
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t AppThreadRef;

//--------------------------------------------------------------------------------------------------
/**
 *  Expected message identifier
 */
//--------------------------------------------------------------------------------------------------
static uint8_t ExpectedMessageId = 0;


//--------------------------------------------------------------------------------------------------
// Utility functions
//--------------------------------------------------------------------------------------------------

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

//--------------------------------------------------------------------------------------------------
/**
 *  Send SAP message through application thread
 */
//--------------------------------------------------------------------------------------------------
static void SendSapMessage
(
    void* param1Ptr,
    void* param2Ptr
)
{
    RsimMessageSending_t* rsimSendingPtr = (RsimMessageSending_t*) param1Ptr;

    // Send message
    LE_ASSERT_OK(le_rsim_SendMessage(rsimSendingPtr->message,
                                     rsimSendingPtr->messageSize,
                                     rsimSendingPtr->callbackPtr,
                                     rsimSendingPtr->context));

    // Release allocated memory
    le_mem_Release(rsimSendingPtr);
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

    LE_DEBUG("Send CONNECT_RESP message:");
    LE_DUMP(message, messageSize);

    // Send message with the application thread
    RsimMessageSending_t* rsimSendingPtr = le_mem_ForceAlloc(RsimMessagesPool);
    memcpy(rsimSendingPtr->message, message, messageSize);
    rsimSendingPtr->messageSize = messageSize;
    rsimSendingPtr->callbackPtr = CallbackHandler;
    rsimSendingPtr->context = (void*)LE_OK;
    le_event_QueueFunctionToThread(AppThreadRef, SendSapMessage, rsimSendingPtr, NULL);
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

    LE_DEBUG("Send STATUS_IND message:");
    LE_DUMP(message, messageSize);

    // Send message with the application thread
    RsimMessageSending_t* rsimSendingPtr = le_mem_ForceAlloc(RsimMessagesPool);
    memcpy(rsimSendingPtr->message, message, messageSize);
    rsimSendingPtr->messageSize = messageSize;
    rsimSendingPtr->callbackPtr = CallbackHandler;
    rsimSendingPtr->context = (void*)LE_OK;
    le_event_QueueFunctionToThread(AppThreadRef, SendSapMessage, rsimSendingPtr, NULL);
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

    LE_DEBUG("Send TRANSFER_ATR_RESP message:");
    LE_DUMP(message, messageSize);

    // Send message with the application thread
    RsimMessageSending_t* rsimSendingPtr = le_mem_ForceAlloc(RsimMessagesPool);
    memcpy(rsimSendingPtr->message, message, messageSize);
    rsimSendingPtr->messageSize = messageSize;
    rsimSendingPtr->callbackPtr = CallbackHandler;
    rsimSendingPtr->context = (void*)LE_OK;
    le_event_QueueFunctionToThread(AppThreadRef, SendSapMessage, rsimSendingPtr, NULL);
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
        message[0]  = SAP_MSGID_TRANSFER_APDU_RESP;     // MsgId (TRANSFER_APDU_RESP)
        message[1]  = 0x02;                             // Parameters number
        message[2]  = 0x00;                             // Reserved
        message[3]  = 0x00;                             // Reserved

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

    LE_DEBUG("Send TRANSFER_APDU_RESP message:");
    LE_DUMP(message, messageSize);

    // Send message with the application thread
    RsimMessageSending_t* rsimSendingPtr = le_mem_ForceAlloc(RsimMessagesPool);
    memcpy(rsimSendingPtr->message, message, messageSize);
    rsimSendingPtr->messageSize = messageSize;
    rsimSendingPtr->callbackPtr = CallbackHandler;
    rsimSendingPtr->context = (void*)LE_OK;
    le_event_QueueFunctionToThread(AppThreadRef, SendSapMessage, rsimSendingPtr, NULL);
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

    LE_DEBUG("Send POWER_SIM_ON_RESP message:");
    LE_DUMP(message, messageSize);

    // Send message with the application thread
    RsimMessageSending_t* rsimSendingPtr = le_mem_ForceAlloc(RsimMessagesPool);
    memcpy(rsimSendingPtr->message, message, messageSize);
    rsimSendingPtr->messageSize = messageSize;
    rsimSendingPtr->callbackPtr = CallbackHandler;
    rsimSendingPtr->context = (void*)LE_OK;
    le_event_QueueFunctionToThread(AppThreadRef, SendSapMessage, rsimSendingPtr, NULL);
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

    LE_DEBUG("Send DISCONNECT_IND message:");
    LE_DUMP(message, messageSize);

    // Send message with the application thread
    RsimMessageSending_t* rsimSendingPtr = le_mem_ForceAlloc(RsimMessagesPool);
    memcpy(rsimSendingPtr->message, message, messageSize);
    rsimSendingPtr->messageSize = messageSize;
    rsimSendingPtr->callbackPtr = CallbackHandler;
    rsimSendingPtr->context = (void*)LE_OK;
    le_event_QueueFunctionToThread(AppThreadRef, SendSapMessage, rsimSendingPtr, NULL);
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

    LE_DEBUG("Send DISCONNECT_RESP message:");
    LE_DUMP(message, messageSize);

    // Send message with the application thread
    RsimMessageSending_t* rsimSendingPtr = le_mem_ForceAlloc(RsimMessagesPool);
    memcpy(rsimSendingPtr->message, message, messageSize);
    rsimSendingPtr->messageSize = messageSize;
    rsimSendingPtr->callbackPtr = CallbackHandler;
    rsimSendingPtr->context = (void*)LE_OK;
    le_event_QueueFunctionToThread(AppThreadRef, SendSapMessage, rsimSendingPtr, NULL);
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
    LE_DEBUG("Received a RSIM message:");
    LE_DUMP(messagePtr, messageNumElements);

    uint8_t msgId = messagePtr[0];

    LE_DEBUG("Received MessageId %d, expected %d", msgId, ExpectedMessageId);
    LE_ASSERT(msgId == ExpectedMessageId);

    switch (msgId)
    {
        case SAP_MSGID_CONNECT_REQ:
            LE_DEBUG("CONNECT_REQ received");
        break;

        case SAP_MSGID_DISCONNECT_REQ:
            LE_DEBUG("DISCONNECT_REQ received");
        break;

        case SAP_MSGID_TRANSFER_APDU_REQ:
            LE_DEBUG("TRANSFER_APDU_REQ received");
        break;

        case SAP_MSGID_TRANSFER_ATR_REQ:
            LE_DEBUG("TRANSFER_ATR_REQ received");
        break;

        case SAP_MSGID_POWER_SIM_OFF_REQ:
            LE_DEBUG("POWER_SIM_OFF_REQ received");
        break;

        case SAP_MSGID_POWER_SIM_ON_REQ:
            LE_DEBUG("POWER_SIM_ON_REQ received");
        break;

        case SAP_MSGID_RESET_SIM_REQ:
            LE_DEBUG("RESET_SIM_REQ received");
        break;

        case SAP_MSGID_TRANSFER_CARD_READER_STATUS_REQ:
        case SAP_MSGID_SET_TRANSPORT_PROTOCOL_REQ:
            LE_ERROR("Unsupported SAP message with id %d received", msgId);
        break;

        default:
            LE_ERROR("Unknown SAP message with id %d received", msgId);
        break;
    }

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(AppSemaphore);
}

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
    le_clk_Time_t timeToWait = {5, 0};

    LE_ASSERT_OK(le_sem_WaitWithTimeOut(AppSemaphore, timeToWait));
}

//--------------------------------------------------------------------------------------------------
/**
 *  Thread used to register handler and to send/receive the RSIM messages
 */
//--------------------------------------------------------------------------------------------------
static void* AppHandler
(
    void* ctxPtr
)
{
    // Connect to the Remote SIM service
    le_rsim_ConnectService();

    // Register handler for RSIM messages notification
    RsimMessageHandlerRef = le_rsim_AddMessageHandler(MessageHandler, NULL);
    LE_ASSERT(NULL != RsimMessageHandlerRef);
    LE_INFO("MessageHandler %p added", RsimMessageHandlerRef);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(AppSemaphore);

    // Run the event loop
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Remove Remote SIM message handler
 */
//--------------------------------------------------------------------------------------------------
static void AppRemoveHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    // Unregister message handler
    LE_INFO("Unregister MessageHandler %p", RsimMessageHandlerRef);
    le_rsim_RemoveMessageHandler(RsimMessageHandlerRef);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(AppSemaphore);
}


//--------------------------------------------------------------------------------------------------
// Test functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the test component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start RSIM test");

    // Simulated ATR response
    uint8_t atrData[22] =
    {
        0x3B, 0x9F, 0x96, 0x80, 0x1F, 0xC7, 0x80, 0x31,
        0xE0, 0x73, 0xFE, 0x21, 0x13, 0x67, 0x93, 0x31,
        0x01, 0x08, 0x01, 0x01, 0x01, 0x72
    };
    size_t atrLength = 22;
    // Simulated APDU response
    uint8_t apduData[2] =
    {
        0x61, 0x2C
    };
    size_t apduLength = 2;

    // Create and expand RSIM messages memory pool
    RsimMessagesPool = le_mem_CreatePool("RsimMessagesPool", sizeof(RsimMessageSending_t));
    le_mem_ExpandPool(RsimMessagesPool, RSIM_EVENTS_POOL_SIZE);

    // Create a semaphore to synchronize the test
    AppSemaphore = le_sem_Create("AppSemaphore", 0);

    // Create a thread to send and receive Remote SIM messages
    AppThreadRef = le_thread_Create("AppThread", AppHandler, NULL);
    le_thread_Start(AppThreadRef);

    // Wait for the thread initialization before continuing the test
    SynchronizeTest();

    // Wait for the remote SIM service connection request
    ExpectedMessageId = SAP_MSGID_CONNECT_REQ;
    SynchronizeTest();

    // Send a CONNECT_RESP message with 'OK, Server can fulfill requirements'
    SendSapConnectResp(SAP_CONNSTATUS_OK, 0);
    // Wait for message sending result
    SynchronizeTest();

    // Send a STATUS_IND message with 'Card reset', triggering an ATR request
    ExpectedMessageId = SAP_MSGID_TRANSFER_ATR_REQ;
    SendSapStatusInd(SAP_STATUSCHANGE_CARD_RESET);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for the message reception
    SynchronizeTest();

    // Send a TRANSFER_ATR_RESP message with 'OK, request processed correctly',
    // triggering an APDU request
    ExpectedMessageId = SAP_MSGID_TRANSFER_APDU_REQ;
    SendSapTransferAtrResp(SAP_RESULTCODE_OK, atrData, atrLength);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for the message reception
    SynchronizeTest();

    // Send a TRANSFER_APDU_RESP message with 'Error, no reason defined',
    // triggering a POWER_SIM_ON request
    ExpectedMessageId = SAP_MSGID_POWER_SIM_ON_REQ;
    SendSapTransferApduResp(SAP_RESULTCODE_ERROR_NO_REASON, NULL, 0);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for the message reception
    SynchronizeTest();

    // Send a POWER_SIM_ON_RESP message with 'OK, request processed correctly',
    // triggering an ATR request
    ExpectedMessageId = SAP_MSGID_TRANSFER_ATR_REQ;
    SendSapPowerSimOnResp(SAP_RESULTCODE_OK);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for the message reception
    SynchronizeTest();

    // Send a TRANSFER_ATR_RESP message with 'OK, request processed correctly',
    // triggering an APDU request
    ExpectedMessageId = SAP_MSGID_TRANSFER_APDU_REQ;
    SendSapTransferAtrResp(SAP_RESULTCODE_OK, atrData, atrLength);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for the message reception
    SynchronizeTest();

    // Send a TRANSFER_APDU_RESP message with 'OK, request processed correctly',
    // triggering a new APDU request
    SendSapTransferApduResp(SAP_RESULTCODE_OK, apduData, apduLength);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for the message reception
    SynchronizeTest();

    // Send a DISCONNECT_IND message with 'Graceful', triggering a disconnection request
    ExpectedMessageId = SAP_MSGID_DISCONNECT_REQ;
    SendSapDisconnectInd(SAP_DISCONNTYPE_GRACEFUL);
    // Wait for message sending result
    SynchronizeTest();

    // Wait for the message reception
    SynchronizeTest();

    // Send a DISCONNECT_RESP message
    SendSapDisconnectResp();
    // Wait for message sending result
    SynchronizeTest();

    // Remove the message handler
    le_event_QueueFunctionToThread(AppThreadRef, AppRemoveHandler, NULL, NULL);
    SynchronizeTest();

    // Delete semaphore
    le_sem_Delete(AppSemaphore);

    // Stop the remote SIM service test application
    exit(EXIT_SUCCESS);
}
