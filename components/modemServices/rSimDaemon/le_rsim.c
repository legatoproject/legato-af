/**
 * @file le_rsim.c
 *
 * RSIM has been moved from the modemDeamon to avoid dead locks during sim and rsim APDU exchanges
 *
 * This file contains the data structures and the source code of the Remote SIM (RSIM) service.
 *
 * The link with the remote SIM card is based on the SIM Access Profile (SAP) protocol. The RSIM
 * service implements the V11r00 SAP specification.
 *
 * All the following SAP features are supported:
 * - Connection management
 * - Transfer APDU
 * - Transfer ATR
 * - Power SIM off
 * - Power SIM on
 * - Reset SIM
 * - Report Status
 * - Error handling
 * Only the following optional features are not supported:
 * - Transfer Card Reader Status
 * - Set Transport Protocol
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_rsim_local.h"
#include "pa_rsim.h"


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
 * Enumeration for the SAP session state
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SAP_SESSION_NOT_CONNECTED = 0,  ///< Initial state, not connected
    SAP_SESSION_CONNECTING,         ///< Negotiating the connection with remote SAP server
    SAP_SESSION_CONNECTED           ///< Connected to remote SAP server
}
SapSessionState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration for the SAP session sub-state used when connected to the remote server
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SAP_SESSION_CONNECTED_IDLE = 0,     ///< Connected and idle
    SAP_SESSION_CONNECTED_APDU,         ///< Processing an APDU request
    SAP_SESSION_CONNECTED_RESET,        ///< Processing a SIM reset request
    SAP_SESSION_CONNECTED_ATR_RESET,    ///< Processing an ATR request for a SIM reset
    SAP_SESSION_CONNECTED_ATR_INSERT,   ///< Processing an ATR request for a SIM insertion
    SAP_SESSION_CONNECTED_POWER_OFF,    ///< Processing a SIM power off request
    SAP_SESSION_CONNECTED_POWER_ON,     ///< Processing a SIM power on request
    SAP_SESSION_CONNECTED_DISCONNECT    ///< Processing a SIM disconnection request
}
SapSessionSubState_t;


//--------------------------------------------------------------------------------------------------
// Data structures
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * RSIM object structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_rsim_MessageHandlerRef_t handlerRef;     ///< Registered message handler reference
    SapSessionState_t           sapState;       ///< Current SAP session state
    SapSessionSubState_t        sapSubState;    ///< Current SAP session sub-state,
                                                ///< used when connected to remote server
    uint16_t                    maxMsgSize;     ///< Maximum message size negotiated
                                                ///< for current SAP session
}
RsimObject_t;

//--------------------------------------------------------------------------------------------------
/**
 * RSIM message structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t message[LE_RSIM_MAX_MSG_SIZE];  ///< Message
    size_t  messageSize;                    ///< Message size
}
RsimMessage_t;

//--------------------------------------------------------------------------------------------------
/**
 * RSIM message sending structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    RsimMessage_t                 rsimMessage;  ///< RSIM message
    le_rsim_CallbackHandlerFunc_t callbackPtr;  ///< Callback response
    void*                         context;      ///< Associated context
}
RsimMessageSending_t;


//--------------------------------------------------------------------------------------------------
// Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Timer securing the SAP connection establishment
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t SapConnectionTimer;

//--------------------------------------------------------------------------------------------------
/**
 * Remote SIM object storing RSIM information. Only one RSIM object is created
 */
//--------------------------------------------------------------------------------------------------
static RsimObject_t RsimObject;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for RSIM messages notification
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t RsimMsgEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Main thread, needed to queue processing functions
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t MainThread;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool used to transfer RSIM messages processing to the main thread
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t RsimMessagesPool;


//--------------------------------------------------------------------------------------------------
// Static functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Notify the new remote SIM card status to the modem and check the result code
 */
//--------------------------------------------------------------------------------------------------
static void NotifySimStatus
(
    pa_rsim_SimStatus_t simStatus
)
{
    // Transmit new SIM status to the modem
    if (LE_OK != pa_rsim_NotifyStatus(simStatus))
    {
        LE_ERROR("Error when sending SIM status %d", simStatus);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a SAP TRANSFER_ATR_REQ message and update the SAP session sub-state
 */
//--------------------------------------------------------------------------------------------------
static void SendSapTransferAtrReq
(
    SapSessionSubState_t sapSubState    ///< New SAP sub-state
)
{
    // Create TRANSFER_ATR_REQ message to transmit
    RsimMessage_t RsimMessage;
    memset(&RsimMessage, 0, sizeof(RsimMessage_t));

    // SAP header
    RsimMessage.message[0] = SAP_MSGID_TRANSFER_ATR_REQ;    // MsgId (TRANSFER_ATR_REQ)
    RsimMessage.message[1] = 0x00;                          // Parameters number
    RsimMessage.message[2] = 0x00;                          // Reserved
    RsimMessage.message[3] = 0x00;                          // Reserved

    // Set message size
    RsimMessage.messageSize = 4;

    // Update SAP session sub-state
    RsimObject.sapSubState = sapSubState;

    // Send TRANSFER_ATR_REQ message by notifying it to the remote SIM server
    LE_DEBUG("Send TRANSFER_ATR_REQ message:");
    LE_DUMP(RsimMessage.message, RsimMessage.messageSize);
    le_event_Report(RsimMsgEventId, &(RsimMessage), sizeof(RsimMessage));
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a SAP TRANSFER_APDU_REQ message and update the SAP session sub-state
 */
//--------------------------------------------------------------------------------------------------
static void SendSapTransferApduReq
(
    pa_rsim_ApduInd_t* apduInd
)
{
    // Create TRANSFER_APDU_REQ message to transmit
    RsimMessage_t RsimMessage;
    size_t size = 0;
    memset(&RsimMessage, 0, sizeof(RsimMessage_t));

    // SAP header
    RsimMessage.message[0] = SAP_MSGID_TRANSFER_APDU_REQ;           // MsgId (TRANSFER_APDU_REQ)
    RsimMessage.message[1] = 0x01;                                  // Parameters number
    RsimMessage.message[2] = 0x00;                                  // Reserved
    RsimMessage.message[3] = 0x00;                                  // Reserved

    // Parameter header
    RsimMessage.message[4] = SAP_PARAMID_COMMAND_APDU;              // Parameter Id (CommandAPDU)
    RsimMessage.message[5] = 0x00;                                  // Reserved
    RsimMessage.message[6] = ((apduInd->apduLength & 0xFF00U) >> MSB_SHIFT);// APDU length (MSB)
    RsimMessage.message[7] = (apduInd->apduLength & 0x00FF);        // APDU length (LSB)

    // Current message size
    size = 8;

    // Parameter value (APDU)
    memcpy(&RsimMessage.message[size], apduInd->apduData, apduInd->apduLength);
    size += apduInd->apduLength;

    // Set message size, which should be 4-byte aligned
    size += (4 - (size % 4));
    RsimMessage.messageSize = size;

    // Check message length
    if (size > RsimObject.maxMsgSize)
    {
        LE_ERROR("SAP message too long! Size=%zu, MaxSize=%d",
                 size, RsimObject.maxMsgSize);

        // Send an APDU response error to the modem
        if (LE_OK != pa_rsim_TransferApduRespError())
        {
            LE_ERROR("Error when transmitting APDU response error");
        }
    }
    else
    {
        // Update SAP session sub-state
        RsimObject.sapSubState = SAP_SESSION_CONNECTED_APDU;

        // Send TRANSFER_APDU_REQ message by notifying it to the remote SIM server
        LE_DEBUG("Send TRANSFER_APDU_REQ message:");
        LE_DUMP(RsimMessage.message, RsimMessage.messageSize);
        le_event_Report(RsimMsgEventId, &(RsimMessage), sizeof(RsimMessage));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a SAP CONNECT_REQ message, start the timer securing the connection establishment and
 * update the SAP session state
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SendSapConnectReq
(
    void
)
{
    // Check if state is coherent
    if (SAP_SESSION_CONNECTED == RsimObject.sapState)
    {
        LE_ERROR("Impossible to connect remote SIM, state %d", RsimObject.sapState);
        return LE_FAULT;
    }

    // Create CONNECT_REQ message to transmit
    RsimMessage_t RsimMessage;
    memset(&RsimMessage, 0, sizeof(RsimMessage_t));

    // SAP header
    RsimMessage.message[0]  = SAP_MSGID_CONNECT_REQ;                    // MsgId (CONNECT_REQ)
    RsimMessage.message[1]  = 0x01;                                     // Parameters number
    RsimMessage.message[2]  = 0x00;                                     // Reserved
    RsimMessage.message[3]  = 0x00;                                     // Reserved

    // Parameter header
    RsimMessage.message[4]  = SAP_PARAMID_MAX_MSG_SIZE;                 // Parameter Id (MaxMsgSize)
    RsimMessage.message[5]  = 0x00;                                     // Reserved
    RsimMessage.message[6]  = 0x00;                                     // Parameter length (MSB)
    RsimMessage.message[7]  = SAP_LENGTH_MAX_MSG_SIZE;                  // Parameter length (LSB)

    // Parameter value (MaxMsgSize)
    RsimMessage.message[8]  = ((RsimObject.maxMsgSize & 0xFF00U) >> MSB_SHIFT); // MaxMsgSize (MSB)
    RsimMessage.message[9]  = (RsimObject.maxMsgSize & 0x00FF);                 // MaxMsgSize (LSB)
    RsimMessage.message[10] = 0x00;                                             // Padding
    RsimMessage.message[11] = 0x00;                                             // Padding

    // Set message size
    RsimMessage.messageSize = 12;

    // Start timer securing the connection establishment
    if (LE_OK != le_timer_Start(SapConnectionTimer))
    {
        LE_ERROR("Impossible to start SapConnectionTimer");
    }

    // Update SAP session state
    RsimObject.sapState = SAP_SESSION_CONNECTING;

    // Send CONNECT_REQ message by notifying it to the remote SIM server
    LE_DEBUG("Send CONNECT_REQ message:");
    LE_DUMP(RsimMessage.message, RsimMessage.messageSize);
    le_event_Report(RsimMsgEventId, &(RsimMessage), sizeof(RsimMessage));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a SAP POWER_SIM_OFF_REQ message and update the SAP session sub-state
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SendSapPowerSimOffReq
(
    void
)
{
    // Check if state is coherent
    if (SAP_SESSION_CONNECTED != RsimObject.sapState)
    {
        LE_ERROR("Impossible to power off remote SIM, state %d", RsimObject.sapState);
        return LE_FAULT;
    }

    // Create POWER_SIM_OFF_REQ message to transmit
    RsimMessage_t RsimMessage;
    memset(&RsimMessage, 0, sizeof(RsimMessage_t));

    // SAP header
    RsimMessage.message[0] = SAP_MSGID_POWER_SIM_OFF_REQ;   // MsgId (POWER_SIM_OFF_REQ)
    RsimMessage.message[1] = 0x00;                          // Parameters number
    RsimMessage.message[2] = 0x00;                          // Reserved
    RsimMessage.message[3] = 0x00;                          // Reserved

    // Set message size
    RsimMessage.messageSize = 4;

    // Update SAP session sub-state
    RsimObject.sapSubState = SAP_SESSION_CONNECTED_POWER_OFF;

    // Send POWER_SIM_OFF_REQ message by notifying it to the remote SIM server
    LE_DEBUG("Send POWER_SIM_OFF_REQ message:");
    LE_DUMP(RsimMessage.message, RsimMessage.messageSize);
    le_event_Report(RsimMsgEventId, &(RsimMessage), sizeof(RsimMessage));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a SAP POWER_SIM_ON_REQ message and update the SAP session sub-state
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SendSapPowerSimOnReq
(
    void
)
{
    // Check if state is coherent
    if (   (SAP_SESSION_CONNECTED != RsimObject.sapState)
        || (SAP_SESSION_CONNECTED_IDLE != RsimObject.sapSubState)
       )
    {
        LE_ERROR("Impossible to power on remote SIM, state %d / sub-state %d",
                 RsimObject.sapState, RsimObject.sapSubState);
        return LE_FAULT;
    }

    // Create POWER_SIM_ON_REQ message to transmit
    RsimMessage_t RsimMessage;
    memset(&RsimMessage, 0, sizeof(RsimMessage_t));

    // SAP header
    RsimMessage.message[0] = SAP_MSGID_POWER_SIM_ON_REQ;    // MsgId (POWER_SIM_ON_REQ)
    RsimMessage.message[1] = 0x00;                          // Parameters number
    RsimMessage.message[2] = 0x00;                          // Reserved
    RsimMessage.message[3] = 0x00;                          // Reserved

    // Set message size
    RsimMessage.messageSize = 4;

    // Update SAP session sub-state
    RsimObject.sapSubState = SAP_SESSION_CONNECTED_POWER_ON;

    // Send POWER_SIM_ON_REQ message by notifying it to the remote SIM server
    LE_DEBUG("Send POWER_SIM_ON_REQ message:");
    LE_DUMP(RsimMessage.message, RsimMessage.messageSize);
    le_event_Report(RsimMsgEventId, &(RsimMessage), sizeof(RsimMessage));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a SAP RESET_SIM_REQ message and update the SAP session sub-state
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SendSapResetSimReq
(
    void
)
{
    // Check if state is coherent
    if (   (SAP_SESSION_CONNECTED != RsimObject.sapState)
        || (SAP_SESSION_CONNECTED_POWER_ON == RsimObject.sapSubState)
        || (SAP_SESSION_CONNECTED_POWER_OFF == RsimObject.sapSubState)
       )
    {
        LE_ERROR("Impossible to reset remote SIM, state %d / sub-state %d",
                 RsimObject.sapState, RsimObject.sapSubState);
        return LE_FAULT;
    }

    // Create RESET_SIM_REQ message to transmit
    RsimMessage_t RsimMessage;
    memset(&RsimMessage, 0, sizeof(RsimMessage_t));

    // SAP header
    RsimMessage.message[0] = SAP_MSGID_RESET_SIM_REQ;   // MsgId (RESET_SIM_REQ)
    RsimMessage.message[1] = 0x00;                      // Parameters number
    RsimMessage.message[2] = 0x00;                      // Reserved
    RsimMessage.message[3] = 0x00;                      // Reserved

    // Set message size
    RsimMessage.messageSize = 4;

    // Update SAP session sub-state
    RsimObject.sapSubState = SAP_SESSION_CONNECTED_RESET;

    // Send RESET_SIM_REQ message by notifying it to the remote SIM server
    LE_DEBUG("Send RESET_SIM_REQ message:");
    LE_DUMP(RsimMessage.message, RsimMessage.messageSize);
    le_event_Report(RsimMsgEventId, &(RsimMessage), sizeof(RsimMessage));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a SAP DISCONNECT_REQ message and update the SAP session sub-state
 */
//--------------------------------------------------------------------------------------------------
static void SendSapDisconnectReq
(
    void
)
{
    // Create DISCONNECT_REQ message to transmit
    RsimMessage_t RsimMessage;
    memset(&RsimMessage, 0, sizeof(RsimMessage_t));

    // SAP header
    RsimMessage.message[0] = SAP_MSGID_DISCONNECT_REQ;  // MsgId (DISCONNECT_REQ)
    RsimMessage.message[1] = 0x00;                      // Parameters number
    RsimMessage.message[2] = 0x00;                      // Reserved
    RsimMessage.message[3] = 0x00;                      // Reserved

    // Set message size
    RsimMessage.messageSize = 4;

    // Update SAP session sub-state
    RsimObject.sapSubState = SAP_SESSION_CONNECTED_DISCONNECT;

    // Send DISCONNECT_REQ message by notifying it to the remote SIM server
    LE_DEBUG("Send DISCONNECT_REQ message:");
    LE_DUMP(RsimMessage.message, RsimMessage.messageSize);
    le_event_Report(RsimMsgEventId, &(RsimMessage), sizeof(RsimMessage));
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a SAP parameter is present in the buffer and if the parameter header is coherent.
 * Set parameterLength to 0 if the parameter length should not be checked.
 *
 * @return
 *  - LE_OK             Message correctly formatted
 *  - LE_FAULT          Message incorrectly formatted
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SapCheckParameter
(
    const uint8_t* messagePtr,      ///< SAP message buffer
    size_t messageNumElements,      ///< SAP message size
    uint8_t parameterId,            ///< Parameter identifier
    uint8_t parameterLength,        ///< Parameter length
    uint8_t parameterNumber         ///< Parameter number in SAP message
)
{
    // Check the required message size against the total message size
    uint8_t minSize = SAP_LENGTH_SAP_HEADER + (parameterNumber * SAP_LENGTH_PARAM);
    if (messageNumElements < minSize)
    {
        LE_ERROR("SAP message too short: %zu bytes, expected %d bytes",
                 messageNumElements, minSize);
        return LE_FAULT;
    }

    // Check the number of parameters (second byte of SAP message)
    if (messagePtr[1] < parameterNumber)
    {
        LE_ERROR("Too few parameters: %d, expected %d", messagePtr[1], parameterNumber);
        return LE_FAULT;
    }

    // Check if parameter identifier is correct in SAP message
    // The identifier is stored in the first byte of the parameter header
    uint8_t identifierByte = SAP_LENGTH_SAP_HEADER + ((parameterNumber - 1) * SAP_LENGTH_PARAM);
    uint8_t identifier = messagePtr[identifierByte];
    if (identifier != parameterId)
    {
        LE_ERROR("Wrong parameter id: %d, expected %d", identifier, parameterId);
        return LE_FAULT;
    }

    // Check parameter length if necessary
    if (parameterLength)
    {
        // The parameter length is stored in the last two bytes of the parameter header
        uint8_t lengthByte1 = SAP_LENGTH_SAP_HEADER + 2 + ((parameterNumber-1) * SAP_LENGTH_PARAM);
        uint8_t lengthByte2 = SAP_LENGTH_SAP_HEADER + 3 + ((parameterNumber-1) * SAP_LENGTH_PARAM);
        uint16_t length = (uint16_t)( ((uint16_t)(messagePtr[lengthByte1] << MSB_SHIFT))
                                      | messagePtr[lengthByte2]);
        if (length != parameterLength)
        {
            LE_ERROR("Wrong parameter length: %d ,expected %d", length, parameterLength);
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a CONNECT_RESP SAP message
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 *  - LE_FORMAT_ERROR   Format error in SAP message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessSapConnectResp
(
    const uint8_t* messagePtr,      ///< SAP message
    size_t messageNumElements       ///< SAP message size
)
{
    // Check if state is coherent
    if (SAP_SESSION_CONNECTING != RsimObject.sapState)
    {
        LE_ERROR("CONNECT_RESP received in incoherent state %d", RsimObject.sapState);
        return LE_FAULT;
    }

    le_result_t result = LE_OK;
    bool noLinkEstablished = false;

    // Analyze CONNECT_RESP message
    // Check if ConnectionStatus parameter (first parameter) is present and correct
    uint8_t parameterNumber =  1;
    if (LE_OK == SapCheckParameter(messagePtr,
                                   messageNumElements,
                                   SAP_PARAMID_CONNECTION_STATUS,
                                   SAP_LENGTH_CONNECTION_STATUS,
                                   parameterNumber))
    {
        uint8_t connectionStatus = messagePtr[8];
        LE_DEBUG("CONNECT_RESP received: ConnectionStatus=%d", connectionStatus);

        // Stop SAP connection timer
        le_timer_Stop(SapConnectionTimer);

        // Update SAP session state
        RsimObject.sapState = SAP_SESSION_NOT_CONNECTED;

        // ConnectionStatus is described in SIM Access Profile specification section 5.2.2
        switch (connectionStatus)
        {
            case SAP_CONNSTATUS_OK:                 // OK, Server can fulfill requirements
            case SAP_CONNSTATUS_OK_ONGOING_CALL:    // OK, ongoing call
                LE_DEBUG("SAP session connected");

                // Connection established, update SAP session state and sub-state
                RsimObject.sapState = SAP_SESSION_CONNECTED;
                RsimObject.sapSubState = SAP_SESSION_CONNECTED_IDLE;
            break;

            case SAP_CONNSTATUS_SERVER_NOK:         // Error, Server unable to establish connection
                // In this case the SAP connection is not established
                // and the client should not try again to establish it
                LE_ERROR("ConnectionStatus: 'Error, Server unable to establish connection'");
                noLinkEstablished = true;
            break;

            case SAP_CONNSTATUS_MAXMSGSIZE_NOK:     // Error, Server does not support
                                                    // maximum message size
                // Check if MaxMsgSize parameter (second parameter) is present and correct
                parameterNumber = 2;
                if (LE_OK == SapCheckParameter(messagePtr,
                                               messageNumElements,
                                               SAP_PARAMID_MAX_MSG_SIZE,
                                               SAP_LENGTH_MAX_MSG_SIZE,
                                               parameterNumber))
                {
                    uint8_t  sizeByte1 = 16;    // MaxMsgSize MSB in the message
                    uint8_t  sizeByte2 = 17;    // MaxMsgSize LSB in the message
                    uint16_t serverMaxMsgSize = (uint16_t)
                                                ( ((uint16_t)(messagePtr[sizeByte1] << MSB_SHIFT))
                                                  | messagePtr[sizeByte2]);

                    LE_DEBUG("Maximum message size not supported by server, %d bytes proposed",
                            serverMaxMsgSize);

                    if (serverMaxMsgSize > LE_RSIM_MAX_MSG_SIZE)
                    {
                        LE_DEBUG("Proposed size is too big (%d > %d), connection not established",
                                serverMaxMsgSize, LE_RSIM_MAX_MSG_SIZE);
                        noLinkEstablished = true;
                    }
                    else if (serverMaxMsgSize < LE_RSIM_MIN_MSG_SIZE)
                    {
                        LE_DEBUG("Proposed size is too small (%d < %d), connection not established",
                                serverMaxMsgSize, LE_RSIM_MIN_MSG_SIZE);
                        noLinkEstablished = true;
                    }
                    else
                    {
                        // Use the maximum message size sent by the server
                        RsimObject.maxMsgSize = serverMaxMsgSize;

                        // Send a new CONNECT_REQ message and update SAP session state
                        SendSapConnectReq();
                    }
                }
                else
                {
                    LE_ERROR("MaxMsgSize missing or improperly formatted in CONNECT_RESP message");
                    noLinkEstablished = true;
                    result = LE_FORMAT_ERROR;
                }
            break;

            case SAP_CONNSTATUS_SMALL_MAXMSGSIZE:   // Error, maximum message size
                                                    // by Client is too small
                // In this case the SAP connection is not established
                // and the client should not try again to establish it
                LE_ERROR("ConnectionStatus: 'Error, maximum message size by Client is too small'");
                noLinkEstablished = true;
            break;

            default:
                LE_ERROR("Unknown ConnectionStatus value %d", connectionStatus);
                noLinkEstablished = true;
                result = LE_FAULT;
            break;
        }
    }
    else
    {
        LE_ERROR("ConnectionStatus missing or improperly formatted in CONNECT_RESP message");
        noLinkEstablished = true;
        result = LE_FORMAT_ERROR;
    }

    if (true == noLinkEstablished)
    {
        // Notify modem that the link was not established
        NotifySimStatus(PA_RSIM_STATUS_NO_LINK);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a STATUS_IND SAP message
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 *  - LE_FORMAT_ERROR   Format error in SAP message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessSapStatusInd
(
    const uint8_t* messagePtr,      ///< SAP message
    size_t messageNumElements       ///< SAP message size
)
{
    // Check if state is coherent
    if (SAP_SESSION_CONNECTED != RsimObject.sapState)
    {
        LE_ERROR("STATUS_IND received in incoherent state %d", RsimObject.sapState);
        return LE_FAULT;
    }

    // Analyze STATUS_IND message
    // Check if StatusChange parameter is present and correct
    if (LE_OK != SapCheckParameter(messagePtr,
                                   messageNumElements,
                                   SAP_PARAMID_STATUS_CHANGE,
                                   SAP_LENGTH_STATUS_CHANGE,
                                   1))
    {
        LE_ERROR("StatusChange missing or improperly formatted in STATUS_IND message");
        return LE_FORMAT_ERROR;
    }

    le_result_t result = LE_OK;
    uint8_t statusChange = messagePtr[8];
    LE_DEBUG("STATUS_IND received: StatusChange=%d", statusChange);

    // Update SAP session sub-state
    RsimObject.sapSubState = SAP_SESSION_CONNECTED_IDLE;

    // StatusChange is described in SIM Access Profile specification section 5.2.8
    switch (statusChange)
    {
        case SAP_STATUSCHANGE_UNKNOWN_ERROR:    // Unknown Error
            LE_DEBUG("StatusChange: 'Unknown error'");

            // Transmit SIM status to the modem
            NotifySimStatus(PA_RSIM_STATUS_UNKNOWN_ERROR);
        break;

        case SAP_STATUSCHANGE_CARD_RESET:       // Card reset
            // Launch ATR request procedure and update SAP session sub-state
            SendSapTransferAtrReq(SAP_SESSION_CONNECTED_ATR_RESET);
        break;

        case SAP_STATUSCHANGE_CARD_NOK:         // Card not accessible
            LE_DEBUG("StatusChange: 'Card not accessible'");

            // Transmit SIM status to the modem
            NotifySimStatus(PA_RSIM_STATUS_NOT_ACCESSIBLE);
        break;

        case SAP_STATUSCHANGE_CARD_REMOVED:     // Card removed
            LE_DEBUG("StatusChange: 'Card removed'");

            // Transmit SIM status to the modem
            NotifySimStatus(PA_RSIM_STATUS_REMOVED);
        break;

        case SAP_STATUSCHANGE_CARD_INSERTED:    // Card inserted
            LE_DEBUG("StatusChange: 'Card inserted'");

            // Launch ATR request procedure and update SAP session sub-state
            SendSapTransferAtrReq(SAP_SESSION_CONNECTED_ATR_INSERT);
        break;

        case SAP_STATUSCHANGE_CARD_RECOVERED:   // Card recovered
            LE_DEBUG("StatusChange: 'Card recovered'");

            // Transmit SIM status to the modem
            NotifySimStatus(PA_RSIM_STATUS_RECOVERED);
        break;

        default:
            LE_ERROR("Unknown StatusChange value %d", statusChange);
            result = LE_FAULT;
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a TRANSFER_ATR_RESP SAP message
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 *  - LE_FORMAT_ERROR   Format error in SAP message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessSapTransferAtrResp
(
    const uint8_t* messagePtr,      ///< SAP message
    size_t messageNumElements       ///< SAP message size
)
{
    // Check if state is coherent
    if (   (SAP_SESSION_CONNECTED != RsimObject.sapState)
        || (   (SAP_SESSION_CONNECTED_ATR_RESET != RsimObject.sapSubState)
            && (SAP_SESSION_CONNECTED_ATR_INSERT != RsimObject.sapSubState)
           )
       )
    {
        LE_ERROR("TRANSFER_ATR_RESP received in incoherent state %d / sub-state %d",
                 RsimObject.sapState, RsimObject.sapSubState);
        return LE_FAULT;
    }

    // Analyze TRANSFER_ATR_RESP message
    // Check if ResultCode parameter (first parameter) is present and correct
    uint8_t parameterNumber = 1;
    if (LE_OK != SapCheckParameter(messagePtr,
                                   messageNumElements,
                                   SAP_PARAMID_RESULT_CODE,
                                   SAP_LENGTH_RESULT_CODE,
                                   parameterNumber))
    {
        LE_ERROR("ResultCode missing or improperly formatted in TRANSFER_ATR_RESP message");
        return LE_FORMAT_ERROR;
    }

    le_result_t result = LE_OK;
    uint8_t resultCode = messagePtr[8];
    LE_DEBUG("TRANSFER_ATR_RESP received: ResultCode=%d", resultCode);

    // ResultCode is described in SIM Access Profile specification section 5.2.4
    switch (resultCode)
    {
        case SAP_RESULTCODE_OK: // OK, request processed correctly
            // Check if ATR parameter (second parameter) is present and correct
            parameterNumber = 2;
            if (LE_OK == SapCheckParameter(messagePtr,
                                           messageNumElements,
                                           SAP_PARAMID_ATR,
                                           0,
                                           parameterNumber))
            {
                uint8_t atrLenByte1  = 14;  // ATR length MSB in the message
                uint8_t atrLenByte2  = 15;  // ATR length LSB in the message
                uint8_t atrFirstByte = 16;  // ATR buffer starts at 16th byte in the message
                uint16_t atrLength = (uint16_t) (((uint16_t)(messagePtr[atrLenByte1] << MSB_SHIFT))
                                                 | messagePtr[atrLenByte2]);

                // Retrieve the SIM status change leading to the ATR
                pa_rsim_SimStatus_t simStatus;
                switch (RsimObject.sapSubState)
                {
                    case SAP_SESSION_CONNECTED_ATR_RESET:
                        simStatus = PA_RSIM_STATUS_RESET;
                    break;

                    case SAP_SESSION_CONNECTED_ATR_INSERT:
                        simStatus = PA_RSIM_STATUS_INSERTED;
                    break;

                    default:
                        simStatus = PA_RSIM_STATUS_UNKNOWN_ERROR;
                        LE_ERROR("Incoherent sub-state %d", RsimObject.sapSubState);
                    break;
                }

                // Transmit the ATR response to the modem
                if (LE_OK != pa_rsim_TransferAtrResp(simStatus,
                                                     &messagePtr[atrFirstByte],
                                                     atrLength))
                {
                    LE_ERROR("Error when transmitting ATR response");
                    result = LE_FAULT;
                }
            }
            else
            {
                LE_ERROR("ATR missing or improperly formatted in TRANSFER_ATR_RESP message");
                result = LE_FORMAT_ERROR;
            }
        break;

        case SAP_RESULTCODE_ERROR_NO_REASON:    // Error, no reason defined
        case SAP_RESULTCODE_ERROR_NO_DATA:      // Error, data not available
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_UNKNOWN_ERROR);
        break;

        case SAP_RESULTCODE_ERROR_CARD_OFF:     // Error, card (already) powered off
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_NOT_ACCESSIBLE);
        break;

        case SAP_RESULTCODE_ERROR_CARD_REMOVED: // Error, card removed
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_REMOVED);
        break;

        default:
            LE_ERROR("Unknown ResultCode value %d for TRANSFER_ATR_RESP", resultCode);
            result = LE_FAULT;
        break;
    }

    // Update SAP session sub-state
    RsimObject.sapSubState = SAP_SESSION_CONNECTED_IDLE;

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a TRANSFER_APDU_RESP SAP message
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 *  - LE_FORMAT_ERROR   Format error in SAP message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessSapTransferApduResp
(
    const uint8_t* messagePtr,      ///< SAP message
    size_t messageNumElements       ///< SAP message size
)
{
    // Check if state is coherent
    if (   (SAP_SESSION_CONNECTED != RsimObject.sapState)
        || (SAP_SESSION_CONNECTED_APDU != RsimObject.sapSubState)
       )
    {
        LE_ERROR("TRANSFER_APDU_RESP received in incoherent state %d / sub-state %d",
                 RsimObject.sapState, RsimObject.sapSubState);
        return LE_FAULT;
    }

    // Analyze TRANSFER_APDU_RESP message
    // Check if ResultCode parameter (first parameter) is present and correct
    uint8_t parameterNumber = 1;
    if (LE_OK != SapCheckParameter(messagePtr,
                                   messageNumElements,
                                   SAP_PARAMID_RESULT_CODE,
                                   SAP_LENGTH_RESULT_CODE,
                                   parameterNumber))
    {
        LE_ERROR("ResultCode missing or improperly formatted in TRANSFER_APDU_RESP message");
        return LE_FORMAT_ERROR;
    }

    le_result_t result = LE_OK;
    uint8_t resultCode = messagePtr[8];
    LE_DEBUG("TRANSFER_APDU_RESP received: ResultCode=%d", resultCode);

    // Update SAP session sub-state
    RsimObject.sapSubState = SAP_SESSION_CONNECTED_IDLE;

    // ResultCode is described in SIM Access Profile specification section 5.2.4
    switch (resultCode)
    {
        case SAP_RESULTCODE_OK: // OK, request processed correctly
            // Check if APDU parameter (second parameter) is present and correct
            parameterNumber = 2;
            if (LE_OK == SapCheckParameter(messagePtr,
                                           messageNumElements,
                                           SAP_PARAMID_COMMAND_APDU,
                                           0,
                                           parameterNumber))
            {
                uint8_t apduLenByte1  = 14; // APDU length MSB in the message
                uint8_t apduLenByte2  = 15; // APDU length LSB in the message
                uint8_t apduFirstByte = 16; // APDU buffer starts at 16th byte in the message
                uint16_t apduLength =
                    (uint16_t) ( ((uint16_t)(messagePtr[apduLenByte1] << MSB_SHIFT))
                                 | messagePtr[apduLenByte2]);
                // Transmit the APDU response to the modem
                if (LE_OK != pa_rsim_TransferApduResp(&messagePtr[apduFirstByte], apduLength))
                {
                    LE_ERROR("Error when transmitting APDU response");
                    result = LE_FAULT;
                }
            }
            else
            {
                LE_ERROR("APDU missing or improperly formatted in TRANSFER_APDU_RESP message");
                result = LE_FORMAT_ERROR;
            }
        break;

        case SAP_RESULTCODE_ERROR_NO_REASON:    // Error, no reason defined
        case SAP_RESULTCODE_ERROR_CARD_NOK:     // Error, card not accessible
        case SAP_RESULTCODE_ERROR_CARD_OFF:     // Error, card (already) powered off
        case SAP_RESULTCODE_ERROR_CARD_REMOVED: // Error, card removed
            // Transmit the APDU response error to the modem
            if (LE_OK != pa_rsim_TransferApduRespError())
            {
                LE_ERROR("Error when transmitting APDU response error");
                result = LE_FAULT;
            }
        break;

        default:
            LE_ERROR("Unknown ResultCode value %d for TRANSFER_APDU_RESP", resultCode);
            result = LE_FAULT;
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a DISCONNECT_IND SAP message
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 *  - LE_FORMAT_ERROR   Format error in SAP message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessSapDisconnectInd
(
    const uint8_t* messagePtr,      ///< SAP message
    size_t messageNumElements       ///< SAP message size
)
{
    // Check if state is coherent
    if (SAP_SESSION_CONNECTED != RsimObject.sapState)
    {
        LE_ERROR("DISCONNECT_IND received in incoherent state %d", RsimObject.sapState);
        return LE_FAULT;
    }

    // Analyze DISCONNECT_IND message
    // Check if DisconnectionType parameter is present and correct
    if (LE_OK != SapCheckParameter(messagePtr,
                                   messageNumElements,
                                   SAP_PARAMID_DISCONNECTION_TYPE,
                                   SAP_LENGTH_DISCONNECTION_TYPE,
                                   1))
    {
        LE_ERROR("DisconnectionType missing or improperly formatted in DISCONNECT_IND message");
        return LE_FORMAT_ERROR;
    }

    le_result_t result = LE_OK;
    uint8_t disconnectionType = messagePtr[8];
    LE_DEBUG("DISCONNECT_IND received: DisconnectionType=%d", disconnectionType);

    // DisconnectionType is described in SIM Access Profile specification section 5.2.3
    switch (disconnectionType)
    {
        case SAP_DISCONNTYPE_GRACEFUL:  // Graceful
            // No APDU stored to send to the server: the graceful disconnection can be
            // treated as an immediate release

            // Send a DISCONNECT_REQ message and update the SAP session sub-state
            SendSapDisconnectReq();

            // Transmit the SIM disconnection indication to the modem
            if (LE_OK != pa_rsim_Disconnect())
            {
                LE_ERROR("Error when transmitting SIM disconnection indication");
                result = LE_FAULT;
            }
        break;

        case SAP_DISCONNTYPE_IMMEDIATE: // Immediate
            // Transmit the SIM disconnection indication to the modem
            if (LE_OK != pa_rsim_Disconnect())
            {
                LE_ERROR("Error when transmitting SIM disconnection indication");
                result = LE_FAULT;
            }

            // Connection closed, update SAP session state and sub-state
            RsimObject.sapState = SAP_SESSION_NOT_CONNECTED;
            RsimObject.sapSubState = SAP_SESSION_CONNECTED_IDLE;
            // Reset maximal message size
            RsimObject.maxMsgSize = LE_RSIM_MAX_MSG_SIZE;
        break;

        default:
            LE_ERROR("Unknown DisconnectionType value %d", disconnectionType);
            result = LE_FAULT;
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a DISCONNECT_RESP SAP message
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessSapDisconnectResp
(
    const uint8_t* messagePtr,      ///< SAP message
    size_t messageNumElements       ///< SAP message size
)
{
    // Check if state is coherent
    if (   (SAP_SESSION_CONNECTED != RsimObject.sapState)
        || (SAP_SESSION_CONNECTED_DISCONNECT != RsimObject.sapSubState)
       )
    {
        LE_ERROR("DISCONNECT_RESP received in incoherent state %d / sub-state %d",
                 RsimObject.sapState, RsimObject.sapSubState);
        return LE_FAULT;
    }

    // No parameter in DISCONNECT_RESP message
    LE_DEBUG("DISCONNECT_RESP received");

    // Connection closed, update SAP session state and sub-state
    RsimObject.sapState = SAP_SESSION_NOT_CONNECTED;
    RsimObject.sapSubState = SAP_SESSION_CONNECTED_IDLE;
    // Reset maximal message size
    RsimObject.maxMsgSize = LE_RSIM_MAX_MSG_SIZE;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a POWER_SIM_OFF_RESP SAP message
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 *  - LE_FORMAT_ERROR   Format error in SAP message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessSapPowerSimOffResp
(
    const uint8_t* messagePtr,      ///< SAP message
    size_t messageNumElements       ///< SAP message size
)
{
    // Check if state is coherent
    if (   (SAP_SESSION_CONNECTED != RsimObject.sapState)
        || (SAP_SESSION_CONNECTED_POWER_OFF != RsimObject.sapSubState)
       )
    {
        LE_ERROR("POWER_SIM_OFF_RESP received in incoherent state %d / sub-state %d",
                 RsimObject.sapState, RsimObject.sapSubState);
        return LE_FAULT;
    }

    // Analyze POWER_SIM_OFF_RESP message
    // Check if ResultCode parameter is present and correct
    if (LE_OK != SapCheckParameter(messagePtr,
                                   messageNumElements,
                                   SAP_PARAMID_RESULT_CODE,
                                   SAP_LENGTH_RESULT_CODE,
                                   1))
    {
        LE_ERROR("ResultCode missing or improperly formatted in POWER_SIM_OFF_RESP message");
        return LE_FORMAT_ERROR;
    }

    le_result_t result = LE_OK;
    uint8_t resultCode = messagePtr[8];
    LE_DEBUG("POWER_SIM_OFF_RESP received: ResultCode=%d", resultCode);

    // Update SAP session sub-state
    RsimObject.sapSubState = SAP_SESSION_CONNECTED_IDLE;

    // ResultCode is described in SIM Access Profile specification section 5.2.4
    switch (resultCode)
    {
        case SAP_RESULTCODE_OK: // OK, request processed correctly
            LE_DEBUG("ResultCode: 'OK, request processed correctly'");
        break;

        case SAP_RESULTCODE_ERROR_NO_REASON:    // Error, no reason defined
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_UNKNOWN_ERROR);
        break;

        case SAP_RESULTCODE_ERROR_CARD_OFF:     // Error, card (already) powered off
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_NOT_ACCESSIBLE);
        break;

        case SAP_RESULTCODE_ERROR_CARD_REMOVED: // Error, card removed
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_REMOVED);
        break;

        default:
            LE_ERROR("Unknown ResultCode value %d for POWER_SIM_OFF_RESP", resultCode);
            result = LE_FAULT;
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a POWER_SIM_ON_RESP SAP message
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 *  - LE_FORMAT_ERROR   Format error in SAP message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessSapPowerSimOnResp
(
    const uint8_t* messagePtr,      ///< SAP message
    size_t messageNumElements       ///< SAP message size
)
{
    // Check if state is coherent
    if (   (SAP_SESSION_CONNECTED != RsimObject.sapState)
        || (SAP_SESSION_CONNECTED_POWER_ON != RsimObject.sapSubState)
       )
    {
        LE_ERROR("POWER_SIM_ON_RESP received in incoherent state %d / sub-state %d",
                 RsimObject.sapState, RsimObject.sapSubState);
        return LE_FAULT;
    }

    // Analyze POWER_SIM_ON_RESP message
    // Check if ResultCode parameter is present and correct
    if (LE_OK != SapCheckParameter(messagePtr,
                                   messageNumElements,
                                   SAP_PARAMID_RESULT_CODE,
                                   SAP_LENGTH_RESULT_CODE,
                                   1))
    {
        LE_ERROR("ResultCode missing or improperly formatted in POWER_SIM_ON_RESP message");
        return LE_FORMAT_ERROR;
    }

    le_result_t result = LE_OK;
    uint8_t resultCode = messagePtr[8];
    LE_DEBUG("POWER_SIM_ON_RESP received: ResultCode=%d", resultCode);

    // Update SAP session sub-state
    RsimObject.sapSubState = SAP_SESSION_CONNECTED_IDLE;

    // ResultCode is described in SIM Access Profile specification section 5.2.4
    switch (resultCode)
    {
        case SAP_RESULTCODE_OK: // OK, request processed correctly
            LE_DEBUG("ResultCode: 'OK, request processed correctly'");

            // Launch ATR request procedure and update SAP session sub-state
            SendSapTransferAtrReq(SAP_SESSION_CONNECTED_ATR_RESET);
        break;

        case SAP_RESULTCODE_ERROR_NO_REASON:    // Error, no reason defined
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_UNKNOWN_ERROR);
        break;

        case SAP_RESULTCODE_ERROR_CARD_NOK:     // Error, card not accessible
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_NOT_ACCESSIBLE);
        break;

        case SAP_RESULTCODE_ERROR_CARD_REMOVED: // Error, card removed
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_REMOVED);
        break;

        case SAP_RESULTCODE_ERROR_CARD_ON:      // Error, card already powered on
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_AVAILABLE);
        break;

        default:
            LE_ERROR("Unknown ResultCode value %d for POWER_SIM_ON_RESP", resultCode);
            result = LE_FAULT;
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a RESET_SIM_RESP SAP message
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 *  - LE_FORMAT_ERROR   Format error in SAP message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessSapResetSimResp
(
    const uint8_t* messagePtr,      ///< SAP message
    size_t messageNumElements       ///< SAP message size
)
{
    // Check if state is coherent
    if (   (SAP_SESSION_CONNECTED != RsimObject.sapState)
        || (SAP_SESSION_CONNECTED_RESET != RsimObject.sapSubState)
       )
    {
        LE_ERROR("RESET_SIM_RESP received in incoherent state %d / sub-state %d",
                 RsimObject.sapState, RsimObject.sapSubState);
        return LE_FAULT;
    }

    // Analyze RESET_SIM_RESP message
    // Check if ResultCode parameter is present and correct
    if (LE_OK != SapCheckParameter(messagePtr,
                                   messageNumElements,
                                   SAP_PARAMID_RESULT_CODE,
                                   SAP_LENGTH_RESULT_CODE,
                                   1))
    {
        LE_ERROR("ResultCode missing or improperly formatted in RESET_SIM_RESP message");
        return LE_FORMAT_ERROR;
    }

    le_result_t result = LE_OK;
    uint8_t resultCode = messagePtr[8];
    LE_DEBUG("RESET_SIM_RESP received: ResultCode=%d", resultCode);

    // Update SAP session sub-state
    RsimObject.sapSubState = SAP_SESSION_CONNECTED_IDLE;

    // ResultCode is described in SIM Access Profile specification section 5.2.4
    switch (resultCode)
    {
        case SAP_RESULTCODE_OK: // OK, request processed correctly
            LE_DEBUG("ResultCode: 'OK, request processed correctly'");

            // Launch ATR request procedure and update SAP session sub-state
            SendSapTransferAtrReq(SAP_SESSION_CONNECTED_ATR_RESET);
        break;

        case SAP_RESULTCODE_ERROR_NO_REASON:    // Error, no reason defined
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_UNKNOWN_ERROR);
        break;

        case SAP_RESULTCODE_ERROR_CARD_NOK:     // Error, card not accessible
        case SAP_RESULTCODE_ERROR_CARD_OFF:     // Error, card (already) powered off
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_NOT_ACCESSIBLE);
        break;

        case SAP_RESULTCODE_ERROR_CARD_REMOVED: // Error, card removed
            // Notify the modem
            NotifySimStatus(PA_RSIM_STATUS_REMOVED);
        break;

        default:
            LE_ERROR("Unknown ResultCode value %d for RESET_SIM_RESP", resultCode);
            result = LE_FAULT;
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process an ERROR_RESP SAP message
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessSapErrorResp
(
    const uint8_t* messagePtr,      ///< SAP message
    size_t messageNumElements       ///< SAP message size
)
{
    le_result_t result = LE_OK;

    // No parameter in ERROR_RESP message, processing is based on current state
    switch (RsimObject.sapState)
    {
        case SAP_SESSION_NOT_CONNECTED:
            LE_ERROR("ERROR_RESP received in incoherent state %d", RsimObject.sapState);
            result = LE_FAULT;
        break;

        case SAP_SESSION_CONNECTING:
            // Connection could not be established, update SAP session state
            RsimObject.sapState = SAP_SESSION_NOT_CONNECTED;
        break;

        case SAP_SESSION_CONNECTED:
            // Process aborted, update SAP session sub-state
            RsimObject.sapSubState = SAP_SESSION_CONNECTED_IDLE;
        break;

        default:
            LE_ERROR("Unknown SAP state %d", RsimObject.sapState);
            result = LE_FAULT;
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process an incoming SAP message
 */
//--------------------------------------------------------------------------------------------------
static void ProcessSapMessage
(
    void* param1Ptr,
    void* param2Ptr
)
{
    le_result_t result;
    RsimMessageSending_t* rsimSendingPtr = (RsimMessageSending_t*) param1Ptr;
    uint8_t* messagePtr = rsimSendingPtr->rsimMessage.message;
    size_t messageNumElements = rsimSendingPtr->rsimMessage.messageSize;
    le_rsim_CallbackHandlerFunc_t callback = rsimSendingPtr->callbackPtr;
    uint8_t msgId = messagePtr[0];

    LE_DEBUG("Process SAP message length (%d):", (int) messageNumElements);
    LE_DUMP(messagePtr, messageNumElements);

    // Process the message based on its identifier
    switch (msgId)
    {
        case SAP_MSGID_CONNECT_RESP:
            result = ProcessSapConnectResp(messagePtr, messageNumElements);
        break;

        case SAP_MSGID_STATUS_IND:
            result = ProcessSapStatusInd(messagePtr, messageNumElements);
        break;

        case SAP_MSGID_TRANSFER_ATR_RESP:
            result = ProcessSapTransferAtrResp(messagePtr, messageNumElements);
        break;

        case SAP_MSGID_TRANSFER_APDU_RESP:
            result = ProcessSapTransferApduResp(messagePtr, messageNumElements);
        break;

        case SAP_MSGID_DISCONNECT_IND:
            result = ProcessSapDisconnectInd(messagePtr, messageNumElements);
        break;

        case SAP_MSGID_DISCONNECT_RESP:
            result = ProcessSapDisconnectResp(messagePtr, messageNumElements);
        break;

        case SAP_MSGID_POWER_SIM_OFF_RESP:
            result = ProcessSapPowerSimOffResp(messagePtr, messageNumElements);
        break;

        case SAP_MSGID_POWER_SIM_ON_RESP:
            result = ProcessSapPowerSimOnResp(messagePtr, messageNumElements);
        break;

        case SAP_MSGID_RESET_SIM_RESP:
            result = ProcessSapResetSimResp(messagePtr, messageNumElements);
        break;

        case SAP_MSGID_ERROR_RESP:
            result = ProcessSapErrorResp(messagePtr, messageNumElements);
        break;

        case SAP_MSGID_TRANSFER_CARD_READER_STATUS_RESP:
        case SAP_MSGID_SET_TRANSPORT_PROTOCOL_RESP:
            LE_ERROR("Unsupported SAP message with id %d received", msgId);
            result = LE_UNSUPPORTED;
        break;

        default:
            LE_ERROR("Unknown SAP message with id %d received", msgId);
            result = LE_BAD_PARAMETER;
        break;
    }

    // Notify sending result through the provided callback
    if (NULL != callback)
    {
        LE_DEBUG("Callback %p called with result %d for message %d", callback, result, msgId);
        callback(msgId, result, rsimSendingPtr->context);
    }
    else
    {
        LE_WARN("No callback found for message %d, result %d", msgId, result);
    }

    // Release allocated memory
    le_mem_Release(rsimSendingPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * SAP connection establishment timer handler
 */
//--------------------------------------------------------------------------------------------------
static void SapConnectionTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("SAP connection establishment timer expired");

    // Send a new CONNECT_REQ message
    SendSapConnectReq();
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer RSIM message notification handler
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerRsimMessageHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    RsimMessage_t* messageEvent = reportPtr;

    le_rsim_MessageHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(messageEvent->message, messageEvent->messageSize, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal SIM action request handler function
 */
//--------------------------------------------------------------------------------------------------
static void SimActionRequestHandler
(
    pa_rsim_Action_t action     ///< SIM action requested by the modem
)
{
    bool processingError = false;

    LE_DEBUG("Received request with SIM action %d", action);

    switch (action)
    {
        case PA_RSIM_CONNECTION:
            // Modem requests a connection with the remote SIM: send a CONNECT_REQ message
            // and update the SAP session state
            if (LE_OK != SendSapConnectReq())
            {
                processingError = true;
            }
        break;

        case PA_RSIM_POWER_DOWN:
            // Modem requests a remote SIM Power Off: send a POWER_SIM_OFF_REQ message
            // and update the SAP session sub-state
            if (LE_OK != SendSapPowerSimOffReq())
            {
                processingError = true;
            }
        break;

        case PA_RSIM_POWER_UP:
            // Modem requests a remote SIM Power On: send a POWER_SIM_ON_REQ message
            // and update the SAP session sub-state
            if (LE_OK != SendSapPowerSimOnReq())
            {
                processingError = true;
            }
        break;

        case PA_RSIM_RESET:
            // Modem requests a remote SIM Reset: send a RESET_SIM_REQ message
            // and update the SAP session sub-state
            if (LE_OK != SendSapResetSimReq())
            {
                processingError = true;
            }
        break;

        case PA_RSIM_DISCONNECTION:
            // Check if state is coherent
            if (   (SAP_SESSION_CONNECTED == RsimObject.sapState)
                && (SAP_SESSION_CONNECTED_IDLE == RsimObject.sapSubState)
               )
            {
                // Modem requests a remote SIM disconnection: send a DISCONNECT_REQ message
                // and update the SAP session sub-state
                SendSapDisconnectReq();
            }
            else
            {
                LE_ERROR("Impossible to disconnect remote SIM, state %d / sub-state %d",
                         RsimObject.sapState, RsimObject.sapSubState);
                processingError = true;
            }
        break;

        default:
            LE_ERROR("Unknown SIM action requested %d", action);
            processingError = true;
        break;
    }

    // Notify the modem if an error occurred during processing
    if (true == processingError)
    {
        NotifySimStatus(PA_RSIM_STATUS_UNKNOWN_ERROR);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal APDU notification handler function
 */
//--------------------------------------------------------------------------------------------------
static void ApduNotificationHandler
(
    pa_rsim_ApduInd_t* apduInd      ///< APDU information sent by the modem
)
{
    LE_DEBUG("APDU received:");
    LE_DUMP(apduInd->apduData, apduInd->apduLength);

    // Check if state is coherent
    if (   (SAP_SESSION_CONNECTED == RsimObject.sapState)
        && (SAP_SESSION_CONNECTED_IDLE == RsimObject.sapSubState)
       )
    {
        // Modem sends an APDU request to the remote SIM: send a TRANSFER_APDU_REQ message
        // and update the SAP session sub-state
        SendSapTransferApduReq(apduInd);
    }
    else
    {
        LE_ERROR("APDU received in incoherent state %d / sub-state %d",
                 RsimObject.sapState, RsimObject.sapSubState);
        // Send an APDU response error to the modem
        if (LE_OK != pa_rsim_TransferApduRespError())
        {
            LE_ERROR("Error when transmitting APDU response error");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the remote SIM card can be used.
 * In this case, indicate to the modem that the remote SIM card is available.
 */
//--------------------------------------------------------------------------------------------------
static void SendSimAvailableInd
(
    void* param1Ptr,
    void* param2Ptr
)
{
    // Check if RSIM should be enabled:
    //  - RSIM is supported
    //  - and the remote SIM is selected
    if ( (true == pa_rsim_IsRsimSupported()) && (true == pa_rsim_IsRemoteSimSelected()) )
    {
        // Indicate to the modem that a remote SIM connection is available
        NotifySimStatus(PA_RSIM_STATUS_AVAILABLE);
    }
}


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the Remote SIM service
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_FAULT          Function failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rsim_Init
(
    void
)
{
    LE_INFO("le_rsim_Init called");

    // Store the main thread for further use
    MainThread = le_thread_GetCurrent();

    // Create an event Id for RSIM messages notification
    RsimMsgEventId = le_event_CreateId("RsimMessage", sizeof(RsimMessage_t));

    // Create and expand RSIM messages memory pool
    RsimMessagesPool = le_mem_CreatePool("RsimMessagesPool", sizeof(RsimMessageSending_t));
    le_mem_ExpandPool(RsimMessagesPool, RSIM_EVENTS_POOL_SIZE);

    // Create timer to secure connection establishment
    SapConnectionTimer = le_timer_Create("SapConnectionTimer");
    le_clk_Time_t interval = {2, 0};   // 2 seconds
    if (   (LE_OK != le_timer_SetInterval(SapConnectionTimer, interval))
        || (LE_OK != le_timer_SetHandler(SapConnectionTimer, SapConnectionTimerHandler))
       )
    {
        LE_ERROR("Impossible to configure SapConnectionTimer");
    }

    // Initialize RSIM object
    memset(&RsimObject, 0, sizeof(RsimObject));
    RsimObject.handlerRef   = NULL;
    RsimObject.sapState     = SAP_SESSION_NOT_CONNECTED;
    RsimObject.sapSubState  = SAP_SESSION_CONNECTED_IDLE;
    RsimObject.maxMsgSize   = LE_RSIM_MAX_MSG_SIZE;

    // Register a handler function for SIM action request indications
    if (NULL == pa_rsim_AddSimActionRequestHandler(SimActionRequestHandler))
    {
        LE_ERROR("pa_rsim_AddSimActionRequestHandler failed");
        return LE_FAULT;
    }

    // Register a handler function for APDU indications
    if (NULL == pa_rsim_AddApduNotificationHandler(ApduNotificationHandler))
    {
        LE_ERROR("pa_rsim_AddApduNotificationHandler failed");
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
// APIs
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler function for RSIM message notification.
 * As soon as the handler is added, the remote SIM server is able to receive messages.
 * If necessary, the modem is therefore notified that a remote SIM card is available.
 * @note Only one handler can be registered
 *
 * @return
 *  - Success: A handler reference, which is only needed for later removal of the handler
 *  - Failure: NULL
 *
 * @note Doesn't return if an invalid handler pointer is passed as a parameter
 */
//--------------------------------------------------------------------------------------------------
le_rsim_MessageHandlerRef_t le_rsim_AddMessageHandler
(
    le_rsim_MessageHandlerFunc_t handlerPtr,    ///< Handler function for message notification
    void* contextPtr                            ///< Handler's context
)
{
    le_event_HandlerRef_t handlerRef;

    // Check if handler pointer is correct
    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler function is NULL!");
        return NULL;
    }

    // Only one handler can be registered
    if (NULL != RsimObject.handlerRef)
    {
        LE_ERROR("RSIM message handler already subscribed");
        return NULL;
    }

    // Add handler
    handlerRef = le_event_AddLayeredHandler("RsimMessageHandler",
                                            RsimMsgEventId,
                                            FirstLayerRsimMessageHandler,
                                            handlerPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);

    // The remote SIM server registered a handler to receive RSIM messages:
    // the connection with the remote SIM card is now established.
    // Use the main thread to indicate to the modem that a remote SIM card is available.
    le_event_QueueFunctionToThread(MainThread, SendSimAvailableInd, NULL, NULL);

    // Store registered handler reference
    RsimObject.handlerRef = (le_rsim_MessageHandlerRef_t)handlerRef;

    return (le_rsim_MessageHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister a handler function
 *
 * @note Doesn't return on failure; there's no need to check the return value for errors
 */
//--------------------------------------------------------------------------------------------------
void le_rsim_RemoveMessageHandler
(
    le_rsim_MessageHandlerRef_t handlerRef  ///< Handler reference
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);

    // Remove stored handler reference
    RsimObject.handlerRef = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called by the Remote SIM server to send a message to the Legato RSIM service
 *
 * @return
 *  - LE_OK             Function succeeded
 *  - LE_BAD_PARAMETER  Message is too long
 *
 * @note The sending process is asynchronous: only the message length is checked by this function
 * before returning a result. A callback function should be passed as a parameter in order to be
 * notified of the message sending result.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rsim_SendMessage
(
    const uint8_t* messagePtr,                  ///< Message to send
    size_t messageNumElements,                  ///< Message size
    le_rsim_CallbackHandlerFunc_t callbackPtr,  ///< Callback for sending result
    void* contextPtr                            ///< Associated context
)
{
    // Check message length
    if (messageNumElements > RsimObject.maxMsgSize)
    {
        LE_ERROR("SAP message too long! Size=%zu, MaxSize=%d",
                 messageNumElements, RsimObject.maxMsgSize);
        return LE_BAD_PARAMETER;
    }

    RsimMessageSending_t* rsimSendingPtr = le_mem_ForceAlloc(RsimMessagesPool);
    memcpy(rsimSendingPtr->rsimMessage.message, messagePtr, messageNumElements);
    rsimSendingPtr->rsimMessage.messageSize = messageNumElements;
    rsimSendingPtr->callbackPtr = callbackPtr;
    rsimSendingPtr->context = contextPtr;

    // Timers are linked to the thread originating their start or stop:
    // all the processing should therefore be done in the same thread to correctly
    // start and stop the timers
    le_event_QueueFunctionToThread(MainThread, ProcessSapMessage, rsimSendingPtr, NULL);

    return LE_OK;
}

