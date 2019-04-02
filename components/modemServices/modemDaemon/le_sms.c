/** @file le_sms.c
 *
 * Legato @ref c_sms implementation.
 *
 * The Modem is initialized to operate in PDU mode: all messages are sent and received in PDU
 * format.
 *
 * The SMS module's initialization installs an internal handler for message reception. This handler
 * decodes the PDU message, then it creates and populates a new message object and finally notifies
 * all the registered client's handlers.
 *
 * All the messages are stored in a le_sms_Msg_t data structure. The message object is always queued
 * to the main 'MsgList' list.
 * In case of listing the received messages (see le_sms_CreateRxMsgList()), the message objects
 * are queued to the 'StoredRxMsgList' as well.
 *
 * The sending case:
 * The message object must be created by the client. The client can populate the message with the
 * 'setters functions' like le_sms_SetDestination() and le_sms_SetText().
 * Then, the client must call le_sms_Send() to actually send the message. le_sms_Send()
 * first verifies the consistency of the main elements of the object like the telephone number or
 * the message length, then it encodes the message in PDU if it is a Text or a Binary message, and
 * finally it forwards the message to the modem for sending.
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
#include "mdmCfgEntries.h"
#include "le_ms_local.h"
#include "watchdogChain.h"


//--------------------------------------------------------------------------------------------------
// Symbols and enums.
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Maximum Message IDs returned by the List SMS messages command.
 *
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_OF_SMS_MSG_IN_STORAGE   256

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Message objects we expect to have at one time.
 * GSM SMS in the SIM and memory, CDMA SMS in the SIM and memory.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_OF_SMS_MSG    (MAX_NUM_OF_SMS_MSG_IN_STORAGE*4)

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Message List objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_OF_LIST    128

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of session objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define SMS_MAX_SESSION 5

//--------------------------------------------------------------------------------------------------
/**
 * SMS command Type.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_SMS_CMD_TYPE_SEND    = 0 ///< Pool and send SMS message.
}
CmdType_t;

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Message structure.
 * Objects of this type are used to define a message.
 *
 * Note: I keep both PDU and UserData since PDU can be requested by the client's app even after
 *       message decoding.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sms_Msg
{
    bool              readonly;                            ///< Flag for Read-Only message.
    bool              inAList;                             ///< Is the message belong to a list?
    le_sms_Format_t   format;                              ///< SMS Message Format.
    le_sms_Type_t     type;                                ///< SMS Message Type.
    uint32_t          storageIdx;                          ///< SMS Message index in storage.
    char              tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES]; ///< Telephone number of the message
                                                           ///< (in text mode),
                                                           ///< or NULL (in PDU mode).
    char              timestamp[LE_SMS_TIMESTAMP_MAX_BYTES]; ///< SMS time stamp (in text mode).
    pa_sms_Pdu_t      pdu;                                 ///< SMS PDU.
    bool              pduReady;                            ///< Is the PDU value ready?
    union
    {
        char          text[LE_SMS_TEXT_MAX_BYTES];         ///< SMS text.
        uint8_t       binary[LE_SMS_BINARY_MAX_BYTES];     ///< SMS binary.
    };
    size_t            userdataLen;                         ///< Length of data associated with SMS.
    ///  formats text or binary
    pa_sms_Protocol_t protocol;                            ///< SMS Protocol (GSM or CDMA).
    int32_t           smsUserCount;                        ///< Current sms user counter.
    bool              delAsked;                            ///< Whether the SMS deletion is asked.
    pa_sms_Storage_t  storage;                             ///< SMS storage location.

    /// SMS Cell Broadcast parameters
    uint16_t          messageId;                           ///< SMS Cell Broadcast message Id.
    uint16_t          messageSerialNumber;                 ///< SMS Cell Broadcast message Serial
                                                           ///< Number.
    /// SMS callback parameters
    void*             callBackPtr;                         ///< Callback response.
    void*             ctxPtr;                              ///< Context.
    le_msg_SessionRef_t sessionRef;                        ///< Client session reference.

    /// SMS Status Report parameters
    uint8_t           messageReference;                             ///< TP Message Reference
    uint8_t           typeOfAddress;                                ///< Type of Address
    char              dischargeTime[LE_SMS_TIMESTAMP_MAX_BYTES];    ///< TP Discharge Time
    uint8_t           status;                                       ///< TP Status
}le_sms_Msg_t;


//--------------------------------------------------------------------------------------------------
/**
 * Data structure to keep a list of the references created with 'CreateRxMsgList' function.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sms_MsgReference
{
    le_sms_MsgRef_t     msgRef;     ///< The message reference.
    le_dls_Link_t       listLink;   ///< Object node link (for msg listing).
}le_sms_MsgReference_t;


//--------------------------------------------------------------------------------------------------
/**
 * List message structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sms_MsgList
{
    le_sms_MsgListRef_t msgListRef;                ///< Message list reference.
    le_msg_SessionRef_t sessionRef;                ///< Client session reference.
    le_dls_List_t       list;                      ///< Link list to insert new message object.
    le_dls_Link_t*      currentLink;               ///< Link list pointed to current message object.
}le_sms_List_t;


//--------------------------------------------------------------------------------------------------
/**
 * Sms message sending command structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    CmdType_t           command;        ///< The command.
    le_sms_MsgRef_t     msgRef;         ///< The message reference.
} CmdRequest_t;


//--------------------------------------------------------------------------------------------------
/**
 * Data structure for message statistics.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool    counting;   ///< Is message counting activated.
    int32_t rxCount;    ///< Number of messages successfully received.
    int32_t rxCbCount;  ///< Number of broadcast messages successfully received.
    int32_t txCount;    ///< Number of messages successfully sent.
}
le_sms_MsgStats_t;


//--------------------------------------------------------------------------------------------------
/**
 * session context node structure used for the SessionCtxList list.
 *
 * @note The goal is to create a sms database to be able to find all sms created or allocated for a
 *       dedicated app and delete them in case of an app crash.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t sessionRef;           ///< Client sessionRef.
    le_dls_List_t       msgRefList;           ///< Message reference list.
    le_dls_List_t       handlerList;          ///< Handler list.
    le_dls_Link_t       link;                 ///< Link for SessionCtxList.
}
SessionCtxNode_t;


//--------------------------------------------------------------------------------------------------
/**
 * HandlerCtx node structure used for the handlerList list.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sms_RxMessageHandlerRef_t  handlerRef;     ///< Handler reference.
    le_sms_RxMessageHandlerFunc_t handlerFuncPtr; ///< Handler function.
    void*                         userContext;    ///< User context.
    SessionCtxNode_t*             sessionCtxPtr;  ///< Session context relative to this handler ctx.
    le_dls_Link_t                 link;           ///< Link for handlerList.
}
HandlerCtxNode_t;


//--------------------------------------------------------------------------------------------------
/**
 * msgRef node structure used for the msgRefList list.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sms_MsgRef_t  msgRef;            ///< The message reference.
    le_dls_Link_t    link;              ///< Link for msgRefList.
}
MsgRefNode_t;


//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for SMS messages.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   MsgPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Message objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t MsgRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Listed SMS messages.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   ListPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for List objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ListRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for message references.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   ReferencePool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for handlers context.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   HandlerPool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for sessions context.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   SessionCtxPool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for msgRef context.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   MsgRefPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for handlers objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t HandlerRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for SMS storage message notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t StorageStatusEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for message sending commands.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t SmsCommandEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore to synchronize threads.
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t SmsSem;

//--------------------------------------------------------------------------------------------------
/**
 * Structure for message statistics.
 */
//--------------------------------------------------------------------------------------------------
static le_sms_MsgStats_t MessageStats;

//--------------------------------------------------------------------------------------------------
/**
 * SMS Status Report activation state.
 */
//--------------------------------------------------------------------------------------------------
static bool StatusReportActivation = false;

//--------------------------------------------------------------------------------------------------
/**
 * list of session context.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t  SessionCtxList;


//--------------------------------------------------------------------------------------------------
/**
 * Read the message counting state
 */
//--------------------------------------------------------------------------------------------------
static bool GetCountingState
(
    void
)
{
    bool countingState;
    le_cfg_IteratorRef_t iteratorRef;

    iteratorRef = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_SMS_PATH);
    countingState = le_cfg_GetBool(iteratorRef, CFG_NODE_COUNTING, true);
    le_cfg_CancelTxn(iteratorRef);

    LE_DEBUG("Retrieved counting state: %d", countingState);

    return countingState;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the message counting state
 */
//--------------------------------------------------------------------------------------------------
static void SetCountingState
(
    bool countState     ///< New message counting state
)
{
    le_cfg_IteratorRef_t iteratorRef;

    LE_DEBUG("New message counting state: %d", countState);

    iteratorRef = le_cfg_CreateWriteTxn(CFG_MODEMSERVICE_SMS_PATH);
    le_cfg_SetBool(iteratorRef, CFG_NODE_COUNTING, countState);
    le_cfg_CommitTxn(iteratorRef);

    MessageStats.counting = countState;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the message count for a message type
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetMessageCount
(
    le_sms_Type_t   messageType,        ///< [IN] Message type
    int32_t*        messageCountPtr     ///< [OUT] Message count pointer
)
{
    le_cfg_IteratorRef_t iteratorRef;
    char countPath[LE_CFG_STR_LEN_BYTES];

    switch (messageType)
    {
        case LE_SMS_TYPE_RX:
            snprintf(countPath, sizeof(countPath), "%s", CFG_NODE_RX_COUNT);
            break;

        case LE_SMS_TYPE_TX:
            snprintf(countPath, sizeof(countPath), "%s", CFG_NODE_TX_COUNT);
            break;

        case LE_SMS_TYPE_BROADCAST_RX:
            snprintf(countPath, sizeof(countPath), "%s", CFG_NODE_RX_CB_COUNT);
            break;

        default:
            LE_ERROR("Unknown message type %d", messageType);
            return LE_FAULT;
    }

    iteratorRef = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_SMS_PATH);
    *messageCountPtr = le_cfg_GetInt(iteratorRef, countPath, 0);
    le_cfg_CancelTxn(iteratorRef);

    LE_DEBUG("Type=%d, count=%d", messageType, *messageCountPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the message count for a message type
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetMessageCount
(
    le_sms_Type_t   messageType,    ///< [IN] Message type
    int32_t         messageCount    ///< [IN] New message count
)
{
    le_cfg_IteratorRef_t iteratorRef;
    char countPath[LE_CFG_STR_LEN_BYTES];

    switch (messageType)
    {
        case LE_SMS_TYPE_RX:
            MessageStats.rxCount = messageCount;
            snprintf(countPath, sizeof(countPath), "%s", CFG_NODE_RX_COUNT);
            break;

        case LE_SMS_TYPE_TX:
            MessageStats.txCount = messageCount;
            snprintf(countPath, sizeof(countPath), "%s", CFG_NODE_TX_COUNT);
            break;

        case LE_SMS_TYPE_BROADCAST_RX:
            MessageStats.rxCbCount = messageCount;
            snprintf(countPath, sizeof(countPath), "%s", CFG_NODE_RX_CB_COUNT);
            break;

        default:
            LE_ERROR("Unknown message type %d", messageType);
            return LE_FAULT;
    }

    iteratorRef = le_cfg_CreateWriteTxn(CFG_MODEMSERVICE_SMS_PATH);
    le_cfg_SetInt(iteratorRef, countPath, messageCount);
    le_cfg_CommitTxn(iteratorRef);

    LE_DEBUG("Type=%d, count=%d", messageType, messageCount);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize message statistics structure
 */
//--------------------------------------------------------------------------------------------------
static void InitializeMessageStatistics
(
    void
)
{
    MessageStats.counting = GetCountingState();
    if (LE_OK != GetMessageCount(LE_SMS_TYPE_RX, &MessageStats.rxCount))
    {
        LE_ERROR("Unable to retrieve received message count");
    }
    if (LE_OK != GetMessageCount(LE_SMS_TYPE_TX, &MessageStats.txCount))
    {
        LE_ERROR("Unable to retrieve sent message count");
    }
    if (LE_OK != GetMessageCount(LE_SMS_TYPE_BROADCAST_RX, &MessageStats.rxCbCount))
    {
        LE_ERROR("Unable to retrieve received broadcast message count");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the SMS Status Report activation state
 */
//--------------------------------------------------------------------------------------------------
static bool GetStatusReportState
(
    void
)
{
    bool statusReportState;
    le_cfg_IteratorRef_t iteratorRef;

    iteratorRef = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_SMS_PATH);
    statusReportState = le_cfg_GetBool(iteratorRef, CFG_NODE_STATUS_REPORT, false);
    le_cfg_CancelTxn(iteratorRef);

    LE_DEBUG("Retrieved Status Report state: %d", statusReportState);

    return statusReportState;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the SMS Status Report activation state
 */
//--------------------------------------------------------------------------------------------------
static void SetStatusReportState
(
    bool statusReportState  ///< New SMS Status Report activation state
)
{
    le_cfg_IteratorRef_t iteratorRef;

    LE_DEBUG("New Status Report state: %d", statusReportState);

    iteratorRef = le_cfg_CreateWriteTxn(CFG_MODEMSERVICE_SMS_PATH);
    le_cfg_SetBool(iteratorRef, CFG_NODE_STATUS_REPORT, statusReportState);
    le_cfg_CommitTxn(iteratorRef);

    StatusReportActivation = statusReportState;
}

//--------------------------------------------------------------------------------------------------
/**
 * Re-initialize a List.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReInitializeList
(
    le_dls_List_t*  msgListPtr
)
{
    le_sms_MsgReference_t*  nodePtr;
    le_dls_Link_t *linkPtr;

    linkPtr = le_dls_Pop(msgListPtr);
    if (linkPtr != NULL)
    {
        do
        {
            // Get the node from MsgList.
            nodePtr = CONTAINER_OF(linkPtr, le_sms_MsgReference_t, listLink);

            le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, nodePtr->msgRef);
            if (msgPtr == NULL)
            {
                LE_CRIT("Invalid reference (%p) provided!", nodePtr);
                return;
            }

            LE_DEBUG("ReInitializeList node 0x%p, obj 0X%p, ref 0x%p, flag %c cpt = %d",
                nodePtr, msgPtr, nodePtr->msgRef,
                (msgPtr->delAsked ? 'Y' : 'N'), msgPtr->smsUserCount);

            if (msgPtr->delAsked)
            {
                le_sms_DeleteFromStorage(nodePtr->msgRef);
            }
            msgPtr->smsUserCount --;

            // Invalidate the Safe Reference.
            le_ref_DeleteRef(MsgRefMap, nodePtr->msgRef);

            // release the message object.
            le_mem_Release(msgPtr);

            // Move to the next node.
            linkPtr = le_dls_Pop(msgListPtr);

            // Release the node.
            le_mem_Release(nodePtr);

        } while (linkPtr != NULL);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create and Populate a new message object from an unknown PDU encoding.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_sms_Msg_t* CreateMessage
(
    uint32_t            storageIdx,     ///< [IN] Storage index.
    pa_sms_Pdu_t*       pduMsgPtr       ///< [IN] PDU message.
)
{
    // Create and populate the SMS message object (it is Read Only).
    le_sms_Msg_t  *newSmsMsgObjPtr;

    // Create the message node.
    newSmsMsgObjPtr = (le_sms_Msg_t*)le_mem_ForceAlloc(MsgPool);

    memset(newSmsMsgObjPtr, 0, sizeof(le_sms_Msg_t));

    newSmsMsgObjPtr->pdu.status = LE_SMS_STATUS_UNKNOWN;
    newSmsMsgObjPtr->pdu.errorCode.code3GPP2 = LE_SMS_ERROR_3GPP2_MAX;
    newSmsMsgObjPtr->pdu.errorCode.rp = LE_SMS_ERROR_3GPP_MAX;
    newSmsMsgObjPtr->pdu.errorCode.tp = LE_SMS_ERROR_3GPP_MAX;
    newSmsMsgObjPtr->readonly = true;
    newSmsMsgObjPtr->storageIdx = storageIdx;
    newSmsMsgObjPtr->type = LE_SMS_TYPE_RX;
    newSmsMsgObjPtr->format = LE_SMS_FORMAT_PDU;

    /* Save the protocol */
    newSmsMsgObjPtr->protocol = pduMsgPtr->protocol;

    /* Copy PDU */
    memcpy(&(newSmsMsgObjPtr->pdu), pduMsgPtr, sizeof(pa_sms_Pdu_t));
    newSmsMsgObjPtr->pduReady = true;

    return (newSmsMsgObjPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Populate a new message object from a SMS-DELIVER PDU.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PopulateSmsDeliver
(
    le_sms_Msg_t*       newSmsMsgObjPtr,    ///< [IN] Message object pointer.
    pa_sms_Pdu_t*       pduMsgPtr,          ///< [IN] PDU message.
    pa_sms_Message_t*   decodedMsgPtr       ///< [IN] Decoded Message.
)
{
    newSmsMsgObjPtr->type = LE_SMS_TYPE_RX;
    newSmsMsgObjPtr->format = decodedMsgPtr->smsDeliver.format;

    switch (newSmsMsgObjPtr->format)
    {
        case LE_SMS_FORMAT_PDU:
            LE_WARN_IF((pduMsgPtr->dataLen > LE_SMS_PDU_MAX_BYTES),
                       "pduMsgPtr->dataLen=%d > LE_SMS_PDU_MAX_BYTES=%d",
                       pduMsgPtr->dataLen, LE_SMS_PDU_MAX_BYTES);
            break;

        case LE_SMS_FORMAT_BINARY:
            if (decodedMsgPtr->smsDeliver.dataLen > LE_SMS_BINARY_MAX_BYTES)
            {
                LE_WARN("smsDeliver.dataLen=%d > LE_SMS_BINARY_MAX_BYTES=%d",
                        decodedMsgPtr->smsDeliver.dataLen, LE_SMS_BINARY_MAX_BYTES);
                newSmsMsgObjPtr->userdataLen = LE_SMS_BINARY_MAX_BYTES;
            }
            else
            {
                newSmsMsgObjPtr->userdataLen = decodedMsgPtr->smsDeliver.dataLen;
            }

            memcpy(newSmsMsgObjPtr->binary,
                   decodedMsgPtr->smsDeliver.data,
                   newSmsMsgObjPtr->userdataLen);
            break;

        case LE_SMS_FORMAT_TEXT:
            if (decodedMsgPtr->smsDeliver.dataLen > LE_SMS_TEXT_MAX_BYTES)
            {
                LE_WARN("smsDeliver.dataLen=%d > LE_SMS_TEXT_MAX_BYTES=%d",
                        decodedMsgPtr->smsDeliver.dataLen, LE_SMS_TEXT_MAX_BYTES);
                newSmsMsgObjPtr->userdataLen = LE_SMS_TEXT_MAX_BYTES;
            }
            else
            {
                newSmsMsgObjPtr->userdataLen = decodedMsgPtr->smsDeliver.dataLen;
            }

            memcpy(newSmsMsgObjPtr->text,
                   decodedMsgPtr->smsDeliver.data,
                   newSmsMsgObjPtr->userdataLen);
            break;

        case LE_SMS_FORMAT_UCS2:
            if (decodedMsgPtr->smsDeliver.dataLen > LE_SMS_UCS2_MAX_BYTES)
            {
                LE_WARN("smsDeliver.dataLen=%d > LE_SMS_UCS2_MAX_BYTES=%d",
                        decodedMsgPtr->smsDeliver.dataLen, LE_SMS_UCS2_MAX_BYTES);
                newSmsMsgObjPtr->userdataLen = LE_SMS_UCS2_MAX_BYTES;
            }
            else
            {
                newSmsMsgObjPtr->userdataLen = decodedMsgPtr->smsDeliver.dataLen;
            }

            memcpy(newSmsMsgObjPtr->binary,
                   decodedMsgPtr->smsDeliver.data,
                   newSmsMsgObjPtr->userdataLen);
            break;

        default:
            LE_CRIT("Unknown SMS format %d", newSmsMsgObjPtr->format);
            return LE_FAULT;
    }

    if (newSmsMsgObjPtr->format != LE_SMS_FORMAT_PDU)
    {
        if (decodedMsgPtr->smsDeliver.option & PA_SMS_OPTIONMASK_OA)
        {
            memcpy(newSmsMsgObjPtr->tel,
                   decodedMsgPtr->smsDeliver.oa,
                   LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
        }
        else
        {
            newSmsMsgObjPtr->tel[0] = '\0';
        }

        if (decodedMsgPtr->smsDeliver.option & PA_SMS_OPTIONMASK_SCTS)
        {
            memcpy(newSmsMsgObjPtr->timestamp,
                   decodedMsgPtr->smsDeliver.scts,
                   LE_SMS_TIMESTAMP_MAX_BYTES);
        }
        else
        {
            newSmsMsgObjPtr->timestamp[0] = '\0';
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Populate a new message object from a Cell Broadcast PDU.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PopulateSmsCellBroadcast
(
    le_sms_Msg_t*       newSmsMsgObjPtr,    ///< [IN] Message object pointer.
    pa_sms_Pdu_t*       pduMsgPtr,          ///< [IN] PDU message.
    pa_sms_Message_t*   decodedMsgPtr       ///< [IN] Decoded Message.
)
{
    newSmsMsgObjPtr->type = LE_SMS_TYPE_BROADCAST_RX;
    newSmsMsgObjPtr->format = decodedMsgPtr->cellBroadcast.format;
    memcpy(newSmsMsgObjPtr->pdu.data, pduMsgPtr->data, pduMsgPtr->dataLen);
    newSmsMsgObjPtr->messageId = decodedMsgPtr->cellBroadcast.mId;
    newSmsMsgObjPtr->messageSerialNumber = decodedMsgPtr->cellBroadcast.serialNum;
    newSmsMsgObjPtr->pduReady = true;

    switch (newSmsMsgObjPtr->format)
    {
        case LE_SMS_FORMAT_PDU:
            LE_WARN_IF((pduMsgPtr->dataLen > LE_SMS_PDU_MAX_BYTES),
                       "pduMsgPtr->dataLen=%d > LE_SMS_PDU_MAX_BYTES=%d",
                       pduMsgPtr->dataLen, LE_SMS_PDU_MAX_BYTES);
            break;

        case LE_SMS_FORMAT_BINARY:
            if (decodedMsgPtr->cellBroadcast.dataLen > LE_SMS_BINARY_MAX_BYTES)
            {
                LE_WARN("cellBroadcast.dataLen=%d > LE_SMS_BINARY_MAX_BYTES=%d",
                        decodedMsgPtr->cellBroadcast.dataLen, LE_SMS_BINARY_MAX_BYTES);
                newSmsMsgObjPtr->userdataLen = LE_SMS_BINARY_MAX_BYTES;
            }
            else
            {
                newSmsMsgObjPtr->userdataLen = decodedMsgPtr->cellBroadcast.dataLen;
            }
            memcpy(newSmsMsgObjPtr->binary,
                   decodedMsgPtr->cellBroadcast.data,
                   newSmsMsgObjPtr->userdataLen);
            break;

        case LE_SMS_FORMAT_TEXT:
            if (decodedMsgPtr->cellBroadcast.dataLen > LE_SMS_TEXT_MAX_BYTES)
            {
                LE_WARN("cellBroadcast.dataLen=%d > LE_SMS_TEXT_MAX_BYTES=%d",
                        decodedMsgPtr->cellBroadcast.dataLen, LE_SMS_TEXT_MAX_BYTES);
                newSmsMsgObjPtr->userdataLen = LE_SMS_TEXT_MAX_BYTES;
            }
            else
            {
                newSmsMsgObjPtr->userdataLen = decodedMsgPtr->cellBroadcast.dataLen;
            }

            memcpy(newSmsMsgObjPtr->text,
                   decodedMsgPtr->cellBroadcast.data,
                   newSmsMsgObjPtr->userdataLen);
            break;

        case LE_SMS_FORMAT_UCS2:
            if (decodedMsgPtr->cellBroadcast.dataLen > LE_SMS_UCS2_MAX_BYTES)
            {
                LE_WARN("cellBroadcast.dataLen=%d > LE_SMS_UCS2_MAX_BYTES=%d",
                        decodedMsgPtr->cellBroadcast.dataLen, LE_SMS_UCS2_MAX_BYTES);
                newSmsMsgObjPtr->userdataLen = LE_SMS_UCS2_MAX_BYTES;
            }
            else
            {
                newSmsMsgObjPtr->userdataLen = decodedMsgPtr->cellBroadcast.dataLen;
            }

            memcpy(newSmsMsgObjPtr->binary,
                   decodedMsgPtr->cellBroadcast.data,
                   newSmsMsgObjPtr->userdataLen);
            break;

        default:
            LE_CRIT("Unknown SMS format %d", newSmsMsgObjPtr->format);
            return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Populate a new message object from a SMS-STATUS-REPORT PDU.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PopulateSmsStatusReport
(
    le_sms_Msg_t*       newSmsMsgObjPtr,    ///< [IN] Message object pointer.
    pa_sms_Pdu_t*       pduMsgPtr,          ///< [IN] PDU message.
    pa_sms_Message_t*   decodedMsgPtr       ///< [IN] Decoded Message.
)
{
    newSmsMsgObjPtr->type = LE_SMS_TYPE_STATUS_REPORT;
    // Format is set by default to text, might change if TP-UD is decoded in the future
    newSmsMsgObjPtr->format = LE_SMS_FORMAT_TEXT;
    newSmsMsgObjPtr->messageReference = decodedMsgPtr->smsStatusReport.mr;
    memcpy(newSmsMsgObjPtr->tel, decodedMsgPtr->smsStatusReport.ra, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
    newSmsMsgObjPtr->typeOfAddress = decodedMsgPtr->smsStatusReport.tora;
    memcpy(newSmsMsgObjPtr->timestamp,
           decodedMsgPtr->smsStatusReport.scts,
           LE_SMS_TIMESTAMP_MAX_BYTES);
    memcpy(newSmsMsgObjPtr->dischargeTime,
           decodedMsgPtr->smsStatusReport.dt,
           LE_SMS_TIMESTAMP_MAX_BYTES);
    newSmsMsgObjPtr->status = decodedMsgPtr->smsStatusReport.st;

    switch (newSmsMsgObjPtr->format)
    {
        case LE_SMS_FORMAT_PDU:
            LE_WARN_IF((pduMsgPtr->dataLen > LE_SMS_PDU_MAX_BYTES),
                       "pduMsgPtr->dataLen=%d > LE_SMS_PDU_MAX_BYTES=%d",
                       pduMsgPtr->dataLen, LE_SMS_PDU_MAX_BYTES);
            break;
        case LE_SMS_FORMAT_TEXT:
            // No user data
            newSmsMsgObjPtr->userdataLen = 0;
            newSmsMsgObjPtr->text[0] = '\0';
            break;
        case LE_SMS_FORMAT_UCS2:
        case LE_SMS_FORMAT_BINARY:
            // No user data
            newSmsMsgObjPtr->userdataLen = 0;
            newSmsMsgObjPtr->binary[0] = '\0';
            break;

        default:
            LE_CRIT("Unknown SMS format %d", newSmsMsgObjPtr->format);
            return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create and Populate a new message object from a PDU.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_sms_Msg_t* CreateAndPopulateMessage
(
    uint32_t            storageIdx,     ///< [IN] Storage index.
    pa_sms_Pdu_t*       pduMsgPtr,      ///< [IN] PDU message.
    pa_sms_Message_t*   decodedMsgPtr   ///< [IN] Decoded Message.
)
{
    // Create and populate the SMS message object (it is Read Only).
    le_sms_Msg_t* newSmsMsgObjPtr;

    // Create the message node.
    newSmsMsgObjPtr = CreateMessage(storageIdx, pduMsgPtr);

    switch (decodedMsgPtr->type)
    {
        case PA_SMS_DELIVER:
            if (LE_OK != PopulateSmsDeliver(newSmsMsgObjPtr, pduMsgPtr, decodedMsgPtr))
            {
                le_mem_Release(newSmsMsgObjPtr);
                newSmsMsgObjPtr = NULL;
            }
            break;

        case PA_SMS_PDU:
            newSmsMsgObjPtr->type = LE_SMS_TYPE_RX;
            newSmsMsgObjPtr->format = LE_SMS_FORMAT_PDU;
            LE_WARN_IF((pduMsgPtr->dataLen > LE_SMS_PDU_MAX_BYTES),
                       "pduMsgPtr->dataLen=%d > LE_SMS_PDU_MAX_BYTES=%d",
                       pduMsgPtr->dataLen, LE_SMS_PDU_MAX_BYTES);
            break;

        case PA_SMS_CELL_BROADCAST:
            if (LE_OK != PopulateSmsCellBroadcast(newSmsMsgObjPtr, pduMsgPtr, decodedMsgPtr))
            {
                le_mem_Release(newSmsMsgObjPtr);
                newSmsMsgObjPtr = NULL;
            }
            break;

        case PA_SMS_STATUS_REPORT:
            if (LE_OK != PopulateSmsStatusReport(newSmsMsgObjPtr, pduMsgPtr, decodedMsgPtr))
            {
                le_mem_Release(newSmsMsgObjPtr);
                newSmsMsgObjPtr = NULL;
            }
            break;

        default:
            LE_CRIT("Unknown or not supported SMS type %d", decodedMsgPtr->type);
            le_mem_Release(newSmsMsgObjPtr);
            return NULL;
    }

    return newSmsMsgObjPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve messages from memory. A new message object is created for each retrieved message and
 * then queued to the list of received messages.
 *
 * @return LE_FAULT          In case of failure
 * @return A positive value  The number of messages read in memory
 */
//--------------------------------------------------------------------------------------------------
static int32_t GetMessagesFromMem
(
    le_sms_List_t      *msgListObjPtr, ///< [OUT]List of received messages.
    pa_sms_Protocol_t   protocol,      ///< [IN] protocol to read.
    uint32_t            numOfMsg,      ///< [IN]Number of message to read from memory.
    uint32_t           *arrayPtr,      ///< [IN]Array of message indexes.
    pa_sms_Storage_t    storage        ///< [IN] Storage used.
)
{
    pa_sms_Pdu_t messagePdu;
    uint32_t     i;
    uint32_t     numOfQueuedMsg=0;

    if (msgListObjPtr == NULL)
    {
        LE_FATAL("msgListObjPtr is NULL !");
    }
    if (arrayPtr == NULL)
    {
        LE_FATAL("arrayPtr is NULL !");
    }

    // Get Unread messages.
    for (i=0 ; i < numOfMsg ; i++)
    {
        // Try to read message for protocol mode.
        le_result_t res;

        le_sem_Wait(SmsSem);
        res = pa_sms_RdPDUMsgFromMem(arrayPtr[i],protocol, storage, &messagePdu);
        le_sem_Post(SmsSem);

        if (res != LE_OK)
        {
            LE_ERROR("pa_sms_RdMsgFromMem failed");
            continue;
        }

        if (messagePdu.dataLen > LE_SMS_PDU_MAX_BYTES)
        {
            LE_ERROR("PDU length out of range (%u) for message %d !",
                            messagePdu.dataLen,
                            arrayPtr[i]);
            continue;
        }

        // Try to decode message.
        pa_sms_Message_t messageConverted;

        if (smsPdu_Decode(messagePdu.protocol,
                          messagePdu.data,
                          messagePdu.dataLen,
                          true,
                          &messageConverted) == LE_OK)
        {
            if (messageConverted.type != PA_SMS_SUBMIT)
            {
                le_sms_Msg_t* newSmsMsgObjPtr = CreateAndPopulateMessage(arrayPtr[i],
                                &messagePdu,
                                &messageConverted);
                if (newSmsMsgObjPtr == NULL)
                {
                    LE_ERROR("Cannot create a new message object! Jump to next one...");
                    continue;
                }
                // Store sms area storage information.
                newSmsMsgObjPtr->storage = storage;
                newSmsMsgObjPtr->inAList = true;

                // Allocate a new node message for the List SMS Message node.
                le_sms_MsgReference_t* newReferencePtr =
                                (le_sms_MsgReference_t*)le_mem_ForceAlloc(ReferencePool);

                // Create a Safe Reference for this Message object.
                newReferencePtr->msgRef = le_ref_CreateRef(MsgRefMap, newSmsMsgObjPtr);
                (newSmsMsgObjPtr->smsUserCount)++;

                LE_DEBUG("create reference node[%p], obj[%p], ref[%p], cpt (%d)",
                    newReferencePtr, newSmsMsgObjPtr,
                    newReferencePtr->msgRef, newSmsMsgObjPtr->smsUserCount);

                newReferencePtr->listLink = LE_DLS_LINK_INIT;
                // Insert the message in the List SMS Message node.
                le_dls_Queue(&(msgListObjPtr->list), &(newReferencePtr->listLink));
                numOfQueuedMsg++;
            }
            else
            {
                LE_WARN("Unexpected message type %d for message %d",
                                messageConverted.type,
                                arrayPtr[i]);
            }
        }
        else
        {
            LE_WARN("Could not decode the message (idx.%d)", arrayPtr[i]);
            le_sms_Msg_t* newSmsMsgObjPtr = CreateMessage(arrayPtr[i], &messagePdu);
            if (newSmsMsgObjPtr == NULL)
            {
                LE_ERROR("Cannot create a new message object! Jump to next one...");
                continue;
            }
            // Store sms area storage information.
            newSmsMsgObjPtr->storage = storage;
            newSmsMsgObjPtr->inAList = true;

            // Allocate a new node message for the List SMS Message node.
            le_sms_MsgReference_t* newReferencePtr =
                            (le_sms_MsgReference_t*)le_mem_ForceAlloc(ReferencePool);
            // Create a Safe Reference for this Message object.
            newReferencePtr->msgRef = le_ref_CreateRef(MsgRefMap, newSmsMsgObjPtr);
            (newSmsMsgObjPtr->smsUserCount)++;

            LE_DEBUG("create reference node[%p], obj[%p], ref[%p], cpt (%d)",
                newReferencePtr, newSmsMsgObjPtr,
                newReferencePtr->msgRef, newSmsMsgObjPtr->smsUserCount);

            newReferencePtr->listLink = LE_DLS_LINK_INIT;
            // Insert the message in the List SMS Message node.
            le_dls_Queue(&(msgListObjPtr->list), &(newReferencePtr->listLink));
            numOfQueuedMsg++;
        }
    }

    return numOfQueuedMsg;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to list the Received Messages present in the message storage.
 *
 * @return LE_NO_MEMORY      The message storage is not available.
 * @return LE_FAULT          The function failed to list messages.
 * @return A positive value  The number of messages present in the storage area, it can be equal to
 *                           zero (no messages).
 */
//--------------------------------------------------------------------------------------------------
static int32_t ListReceivedMessages
(
    le_sms_List_t      *msgListObjPtr,  ///< List of received messages.
    pa_sms_Protocol_t   protocol,       ///< [IN] protocol to read.
    le_sms_Status_t     status,         ///< [IN] status to read.
    pa_sms_Storage_t    storage         ///< [IN] Storage used.
)
{
    le_result_t  result = LE_OK;
    uint32_t     numTot;
    uint32_t     msgCount = 0;
    /* Arrays to store IDs of messages saved in the storage area.*/
    uint32_t     idxArray[MAX_NUM_OF_SMS_MSG_IN_STORAGE]={0};

    if (msgListObjPtr == NULL)
    {
        LE_FATAL("msgListObjPtr is NULL !");
    }

    /* Get Indexes. */
    le_sem_Wait(SmsSem);
    result = pa_sms_ListMsgFromMem(status, protocol, &numTot, idxArray, storage);
    le_sem_Post(SmsSem);

    if (result != LE_OK)
    {
        LE_ERROR("pa_sms_ListMsgFromMem failed");
        return result;
    }

    /* Retrieve messages. */
    if ((numTot > 0) && (numTot < MAX_NUM_OF_SMS_MSG_IN_STORAGE))
    {
        int32_t retValue;

        retValue = GetMessagesFromMem(msgListObjPtr, protocol, numTot, idxArray, storage);
        if(retValue == LE_FAULT)
        {
            LE_WARN("No message retrieve for protocol %d", protocol);
        }
        else
        {
            msgCount = retValue;
        }

    }
    else if (numTot == 0)
    {
        result = LE_OK;
        msgCount = 0;
    }
    else
    {
        LE_ERROR("Too much SMS to read %d", numTot);
        return LE_FAULT;
    }

    return msgCount;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to list the Received Messages present in the message storage.
 *
 * @return
 * - LE_FAULT          The function failed to list messages.
 * - A positive value  The number of messages present in the storage area, it can be equal to
 *                       zero (no messages).
 */
//--------------------------------------------------------------------------------------------------
static int32_t ListAllReceivedMessages
(
    le_sms_List_t* msgListObjPtr ///< List of received messages.
)
{
    int32_t    res, msgCount = 0;
    le_sim_States_t state;

    if (msgListObjPtr == NULL)
    {
        LE_FATAL("msgListObjPtr is NULL !");
    }

    // Check if a SIM is available to list SMS present.
    if (pa_sim_GetState(&state) == LE_OK)
    {
        if(state == LE_SIM_READY)
        {
            // Retrieve message Read for protocol GSM in SIM storage.
            res = ListReceivedMessages(msgListObjPtr, PA_SMS_PROTOCOL_GSM, LE_SMS_RX_READ,
                            PA_SMS_STORAGE_SIM);
            if (res < 0)
            {
                LE_ERROR("SMS read Sim storage is not available, return %d",res);
                return LE_FAULT;
            }
            msgCount += res;

            // Retrieve message unRead for protocol GSM in SIM storage.
            res = ListReceivedMessages(msgListObjPtr, PA_SMS_PROTOCOL_GSM, LE_SMS_RX_UNREAD,
                            PA_SMS_STORAGE_SIM);
            if (res < 0)
            {
                LE_ERROR("SMS unread Sim storage is not available, return %d",res);
                return LE_FAULT;
            }
            msgCount += res;

            // GSM SMS memory storage is not available if SIM is not ready.
            // Retrieve message Read for protocol GSM in memory storage.
            res = ListReceivedMessages(msgListObjPtr, PA_SMS_PROTOCOL_GSM, LE_SMS_RX_READ,
                            PA_SMS_STORAGE_NV);
            if (res < 0)
            {
                LE_ERROR("SMS read memory storage is not available, return %d",res);
                return LE_FAULT;
            }
            msgCount += res;

            // GSM SMS memory storage is not available if SIM is not ready.
            // Retrieve message unRead for protocol GSM in memory storage.
            res = ListReceivedMessages(msgListObjPtr, PA_SMS_PROTOCOL_GSM, LE_SMS_RX_UNREAD,
                            PA_SMS_STORAGE_NV);
            if (res < 0)
            {
                LE_ERROR("SMS unread memory storage is not available, return %d",res);
                return LE_FAULT;
            }
            msgCount += res;

            // No way to know if CDMA SMS sim storage is available.
            // Retrieve message Read for protocol CDMA.
            res = ListReceivedMessages(msgListObjPtr, PA_SMS_PROTOCOL_CDMA, LE_SMS_RX_READ,
                            PA_SMS_STORAGE_SIM);
            if (res < 0)
            {
                LE_WARN("SMS CDMA read sim storage is not available, return %d", res);
            }
            else
            {
                msgCount += res;
            }

            // No way to know if CDMA SMS sim storage is available.
            // Retrieve message unRead for protocol CDMA.
            res = ListReceivedMessages(msgListObjPtr, PA_SMS_PROTOCOL_CDMA, LE_SMS_RX_UNREAD,
                            PA_SMS_STORAGE_SIM);
            if (res < 0)
            {
                LE_WARN("SMS CDMA unread sim storage is not available, return %d", res);
            }
            else
            {
                msgCount += res;
            }
        }
        else
        {
            LE_WARN("Sim not ready");
        }
    }
    else
    {
        LE_WARN("Sim not present");
    }

    // No way to know if CDMA SMS memory storage is available.
    // Retrieve message Read for protocol CDMA.
    res = ListReceivedMessages(msgListObjPtr, PA_SMS_PROTOCOL_CDMA, LE_SMS_RX_READ,
                    PA_SMS_STORAGE_NV);
    if (res < 0)
    {
        LE_WARN("SMS CDMA read memory storage is not available, return %d", res);
    }
    else
    {
        msgCount += res;
    }

    // No way to know if CDMA SMS memory storage is available.
    // Retrieve message unRead for protocol CDMA.
    res = ListReceivedMessages(msgListObjPtr, PA_SMS_PROTOCOL_CDMA, LE_SMS_RX_UNREAD,
                    PA_SMS_STORAGE_NV);
    if (res < 0)
    {
        LE_WARN("SMS CDMA unread memory storage is not available, return %d", res);
    }
    else
    {
        msgCount += res;
    }

    // Check if almost one SMS storage has been read.
    return msgCount;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to prepare a message to be sent by converting its content to PDU data.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EncodeMessageToPdu
(
    le_sms_Msg_t* msgPtr         ///< [IN] The message to encode.
)
{
    le_result_t result = LE_FAULT;
    smsPdu_DataToEncode_t data;

    if (msgPtr == NULL)
    {
        LE_ERROR("msgPtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    memset(&data, 0, sizeof(data));
    data.protocol = msgPtr->protocol;
    data.addressPtr = msgPtr->tel;
    data.statusReport = StatusReportActivation;

    switch (msgPtr->format)
    {
        case LE_SMS_FORMAT_TEXT:
            LE_DEBUG("Try to encode Text Msg %p, tel.%s, text.%s, userdataLen %zd, protocol %d",
                     msgPtr, msgPtr->tel, msgPtr->text, msgPtr->userdataLen, msgPtr->protocol);

            /* @todo send split messages */
            data.messagePtr = (const uint8_t*)msgPtr->text;
            data.length = msgPtr->userdataLen;
            data.encoding = SMSPDU_7_BITS;
            data.messageType = PA_SMS_SUBMIT;
            result = smsPdu_Encode(&data, &(msgPtr->pdu));
            break;

        case LE_SMS_FORMAT_BINARY:
            LE_DEBUG("Try to encode Binary Msg.%p, tel.%s, binary.%p, userdataLen.%zd, protocol %d",
                     msgPtr, msgPtr->tel, msgPtr->binary, msgPtr->userdataLen, msgPtr->protocol);

            /* @todo send split messages */
            data.messagePtr = msgPtr->binary;
            data.length = msgPtr->userdataLen;
            data.encoding = SMSPDU_8_BITS;
            data.messageType = PA_SMS_SUBMIT;
            result = smsPdu_Encode(&data, &(msgPtr->pdu));
            break;

        case LE_SMS_FORMAT_PDU:
            // No need to encode
            result = LE_OK;
            break;

        case LE_SMS_FORMAT_UCS2:
            LE_DEBUG("Try to encode UCS2 Msg.%p, tel.%s, binary.%p, userdataLen.%zd, protocol %d",
                     msgPtr, msgPtr->tel, msgPtr->binary, msgPtr->userdataLen, msgPtr->protocol);

            /* @todo send split messages */
            data.messagePtr = msgPtr->binary;
            data.length = msgPtr->userdataLen;
            data.encoding = SMSPDU_UCS2_16_BITS;
            data.messageType = PA_SMS_SUBMIT;
            result = smsPdu_Encode(&data, &(msgPtr->pdu));
            break;

        case LE_SMS_FORMAT_UNKNOWN:
        default:
            // Unknown format
            result = LE_FAULT;
            break;
    }

    if (result != LE_OK)
    {
        LE_WARN("Failed to encode the message");
    }
    else
    {
        msgPtr->pduReady = true;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a session context.
 *
 */
//--------------------------------------------------------------------------------------------------
static SessionCtxNode_t* CreateSessionCtx
(
    void
)
{
    // Create the session context
    SessionCtxNode_t* sessionCtxPtr = le_mem_ForceAlloc(SessionCtxPool);

    sessionCtxPtr->sessionRef = le_sms_GetClientSessionRef();
    sessionCtxPtr->link = LE_DLS_LINK_INIT;
    sessionCtxPtr->msgRefList = LE_DLS_LIST_INIT;
    sessionCtxPtr->handlerList = LE_DLS_LIST_INIT;

    le_dls_Queue(&SessionCtxList, &(sessionCtxPtr->link));

    LE_DEBUG("Context for sessionRef %p created at %p", sessionCtxPtr->sessionRef, sessionCtxPtr);

    return sessionCtxPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a session context.
 *
 */
//--------------------------------------------------------------------------------------------------
static SessionCtxNode_t* GetSessionCtx
(
    le_msg_SessionRef_t sessionRef   ///< [IN] Message session reference.
)
{
    if (NULL == sessionRef)
    {
        LE_ERROR("Invalid reference (%p) provided", sessionRef);
        return NULL;
    }

    SessionCtxNode_t* sessionCtxPtr = NULL;
    le_dls_Link_t* linkPtr = le_dls_Peek(&SessionCtxList);

    while (linkPtr)
    {
        SessionCtxNode_t* sessionCtxTmpPtr = CONTAINER_OF(linkPtr, SessionCtxNode_t, link);
        linkPtr = le_dls_PeekNext(&SessionCtxList, linkPtr);
        if (sessionCtxTmpPtr->sessionRef == sessionRef)
        {
            sessionCtxPtr = sessionCtxTmpPtr;
            LE_DEBUG("sessionCtx %p found for the sessionRef %p", sessionCtxPtr, sessionRef);
            return sessionCtxPtr;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the session context from the msgRef.
 *
 */
//--------------------------------------------------------------------------------------------------
static SessionCtxNode_t* GetSessionCtxFromMsgRef
(
    le_sms_MsgRef_t msgRef   ///< [IN] Message reference.
)
{
    if (NULL == msgRef)
    {
        LE_ERROR("Invalid reference (%p) provided", msgRef);
        return NULL;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(&SessionCtxList);

    // For all sessions, search the msgRef
    while (linkPtr)
    {
        SessionCtxNode_t* sessionCtxPtr = CONTAINER_OF(linkPtr, SessionCtxNode_t, link);
        linkPtr = le_dls_PeekNext(&SessionCtxList, linkPtr);
        le_dls_Link_t* linkSessionCtxPtr = le_dls_Peek(&(sessionCtxPtr->msgRefList));

        while (linkSessionCtxPtr)
        {
            MsgRefNode_t* msgRefNodePtr = CONTAINER_OF(linkSessionCtxPtr, MsgRefNode_t, link);
            linkSessionCtxPtr = le_dls_PeekNext(&(sessionCtxPtr->msgRefList), linkSessionCtxPtr);
            if (msgRefNodePtr->msgRef == msgRef)
            {
                LE_DEBUG("sessionCtx %p found for msgRef %p", sessionCtxPtr, msgRef);
                return sessionCtxPtr;
            }
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the message reference for a client.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_sms_MsgRef_t SetMsgRefForSessionCtx
(
    le_sms_Msg_t*     msgPtr,       ///< [IN] SMS structure pointer.
    SessionCtxNode_t* sessionCtxPtr ///< [IN] Session context pointer.
)
{
    if (NULL == msgPtr)
    {
        LE_ERROR("Invalid reference (%p) provided", msgPtr);
        return NULL;
    }

    if (NULL == sessionCtxPtr)
    {
        LE_ERROR("Invalid reference (%p) provided", sessionCtxPtr);
        return NULL;
    }

    MsgRefNode_t* msgNodePtr = le_mem_ForceAlloc(MsgRefPool);

    msgNodePtr->msgRef = le_ref_CreateRef(MsgRefMap, msgPtr);
    msgNodePtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&(sessionCtxPtr->msgRefList), &(msgNodePtr->link));

    LE_DEBUG("Set %p for message %p and session %p", msgNodePtr->msgRef, msgPtr, sessionCtxPtr);

    return msgNodePtr->msgRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove a message reference from a session context
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveMsgRefFromSessionCtx
(
    SessionCtxNode_t*  sessionCtxPtr,    ///< [IN] Session context pointer.
    le_sms_MsgRef_t    msgRef            ///< [IN] Message reference.
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&sessionCtxPtr->msgRefList);

    while (linkPtr)
    {
        MsgRefNode_t* msgRefPtr = CONTAINER_OF(linkPtr, MsgRefNode_t, link);

        linkPtr = le_dls_PeekNext(&sessionCtxPtr->msgRefList, linkPtr);

        if (msgRefPtr->msgRef == msgRef)
        {
            // Remove this node.
            LE_DEBUG("Remove msgRef %p from sessionCtxPtr %p", msgRef, sessionCtxPtr);
            le_ref_DeleteRef(MsgRefMap, msgRefPtr->msgRef);
            le_dls_Remove(&(sessionCtxPtr->msgRefList), &(msgRefPtr->link));
            le_mem_Release(msgRefPtr);
            return;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Call all subscribed handlers.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MessageHandlers
(
    le_sms_Msg_t* msgPtr ///< [IN] SMS structure pointer.
)
{
    bool newMessage = true;

    le_dls_Link_t* linkPtr = le_dls_PeekTail(&SessionCtxList);

    // For all sessions, call all handlers.
    while (linkPtr)
    {
        SessionCtxNode_t* sessionCtxPtr = CONTAINER_OF(linkPtr, SessionCtxNode_t, link);

        linkPtr = le_dls_PeekPrev(&SessionCtxList, linkPtr);

        // Peek the tail of the handlers list: this is important for handlers subscribed by
        // reference for modemDaemon.
        le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(sessionCtxPtr->handlerList));

        // Nothing to do if no handler for the current client session.
        if (linkHandlerPtr)
        {
            // Iterate on the handler list of the session.
            while (linkHandlerPtr)
            {
                // Create new msgRef for each client handler.
                le_sms_MsgRef_t msgRef = SetMsgRefForSessionCtx(msgPtr, sessionCtxPtr);

                // If msgRef exists, call the handler.
                if (NULL != msgRef)
                {
                    if (newMessage)
                    {
                        newMessage = false;
                        msgPtr->smsUserCount = 1;
                    }
                    else
                    {
                        msgPtr->smsUserCount++;
                    }

                    HandlerCtxNode_t * handlerCtxPtr = CONTAINER_OF(linkHandlerPtr,
                                                                    HandlerCtxNode_t,
                                                                    link);

                    linkHandlerPtr = le_dls_PeekPrev(&(sessionCtxPtr->handlerList), linkHandlerPtr);

                    // Call the handler.
                    LE_DEBUG("call handler for sessionRef %p, msgRef %p", sessionCtxPtr->sessionRef,
                                                                         msgRef);

                    handlerCtxPtr->handlerFuncPtr(msgRef, handlerCtxPtr->userContext);
                }
                else
                {
                    LE_ERROR("Null msgRef !!!");
                }
            }
        }
        else
        {
            LE_DEBUG("sessionCtxPtr %p has no handler", sessionCtxPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * New SMS message handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NewSmsHandler
(
    pa_sms_NewMessageIndication_t *newMessageIndicationPtr
)
{
    pa_sms_Pdu_t messagePdu;
    memset(&messagePdu, 0, sizeof(pa_sms_Pdu_t));
    le_result_t res = LE_OK;
    bool handlerPresent = false;
    bool smscInfoPresent = true;

    le_dls_Link_t* linkPtr = le_dls_Peek(&SessionCtxList);

    // For all sessions, check if any handlers are present.
    while (linkPtr)
    {
        SessionCtxNode_t* sessionCtxPtr = CONTAINER_OF(linkPtr, SessionCtxNode_t, link);
        linkPtr = le_dls_PeekNext(&SessionCtxList, linkPtr);
        le_dls_Link_t* linkHandlerPtr = le_dls_Peek(&(sessionCtxPtr->handlerList));

        if (linkHandlerPtr)
        {
            LE_DEBUG("Handler has been subscribed for the session (%p)", sessionCtxPtr);
            handlerPresent = true;
            break;
        }
    }

    LE_DEBUG("Handler Function called with message ID %d with protocol %d, Storage %d",
             newMessageIndicationPtr->msgIndex,
             newMessageIndicationPtr->protocol,
             newMessageIndicationPtr->storage);

    if (newMessageIndicationPtr->storage != PA_SMS_STORAGE_NONE)
    {
        le_sem_Wait(SmsSem);
        res = pa_sms_RdPDUMsgFromMem(newMessageIndicationPtr->msgIndex,
                                     newMessageIndicationPtr->protocol,
                                     newMessageIndicationPtr->storage,
                                     &messagePdu);
        le_sem_Post(SmsSem);
    }
    else
    {
        LE_DEBUG("SMS Cell Broadcast GW '%c', CDMA Format '%c', GSM Format '%c'",
                 (newMessageIndicationPtr->protocol == PA_SMS_PROTOCOL_GW_CB ? 'Y' : 'N'),
                 (newMessageIndicationPtr->protocol == PA_SMS_PROTOCOL_CDMA ? 'Y' : 'N'),
                 (newMessageIndicationPtr->protocol == PA_SMS_PROTOCOL_GSM ? 'Y' : 'N'));

        memcpy(messagePdu.data, newMessageIndicationPtr->pduCB, LE_SMS_PDU_MAX_BYTES);
        messagePdu.dataLen = newMessageIndicationPtr->pduLen;
        messagePdu.protocol = newMessageIndicationPtr->protocol;

        // No SMSC information in PDUs which are not stored
        smscInfoPresent = false;
    }

    if (LE_OK != res)
    {
        LE_ERROR("pa_sms_RdPDUMsgFromMem failed");
        return;
    }

    pa_sms_Message_t messageConverted;
    le_sms_Msg_t* newSmsMsgObjPtr = NULL;

    if (messagePdu.dataLen > LE_SMS_PDU_MAX_BYTES)
    {
        LE_ERROR("PDU length out of range (%u) !", messagePdu.dataLen);
    }

    // Try to decode message.
    res = smsPdu_Decode(messagePdu.protocol,
                        messagePdu.data,
                        messagePdu.dataLen,
                        smscInfoPresent,
                        &messageConverted);
    if (   (LE_OK == res)
        && (   (messageConverted.type == PA_SMS_DELIVER)
            || (messageConverted.type == PA_SMS_CELL_BROADCAST)
            || (messageConverted.type == PA_SMS_STATUS_REPORT)))
    {
        newSmsMsgObjPtr = CreateAndPopulateMessage(newMessageIndicationPtr->msgIndex,
                                                   &messagePdu,
                                                   &messageConverted);
    }
    else
    {
        LE_DEBUG("Could not decode the message");
        newSmsMsgObjPtr = CreateMessage(newMessageIndicationPtr->msgIndex, &messagePdu);
    }

    if (newSmsMsgObjPtr == NULL)
    {
        LE_CRIT("Cannot create a new message object, no report!");
        return;
    }

    newSmsMsgObjPtr->storage = newMessageIndicationPtr->storage;

    // Update received message count if necessary
    if (MessageStats.counting)
    {
        if (LE_SMS_TYPE_RX == newSmsMsgObjPtr->type)
        {
            SetMessageCount(newSmsMsgObjPtr->type, MessageStats.rxCount + 1);
        }
        else if (LE_SMS_TYPE_BROADCAST_RX == newSmsMsgObjPtr->type)
        {
            SetMessageCount(newSmsMsgObjPtr->type, MessageStats.rxCbCount + 1);
        }
        else if (LE_SMS_TYPE_STATUS_REPORT == newSmsMsgObjPtr->type)
        {
            // SMS Status Report are not considered in received messages
        }
        else
        {
            LE_ERROR("Unexpected message type %d received", newSmsMsgObjPtr->type);
        }
    }

    // If no client sessions are subscribed for handler then free memory and return
    if (false == handlerPresent)
    {
        LE_DEBUG("No client sessions are subscribed for handler.");
        le_mem_Release(newSmsMsgObjPtr);
        return;
    }

    // Notify all the registered client's handlers with own reference.
    MessageHandlers(newSmsMsgObjPtr);

    LE_DEBUG("All the registered client's handlers notified with objPtr %p, Obj %p",
             &newSmsMsgObjPtr, newSmsMsgObjPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer SMS storage handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerStorageSmsHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_sms_FullStorageHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    le_sms_Storage_t storage = *(le_sms_Storage_t *)reportPtr;

    clientHandlerFunc(storage, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * SMS storage indication handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void StorageIndicationHandler
(
    pa_sms_StorageStatusInd_t *storageMessageIndicationPtr
)
{
    le_sms_Storage_t storage = LE_SMS_STORAGE_MAX;

    LE_DEBUG("SMS storage is full : Storage SIM '%c', NV '%c'",
               (storageMessageIndicationPtr->storage == PA_SMS_STORAGE_SIM ? 'Y' : 'N'),
               (storageMessageIndicationPtr->storage == PA_SMS_STORAGE_NV ? 'Y' : 'N'));

    switch(storageMessageIndicationPtr->storage)
    {
        case PA_SMS_STORAGE_NV:
        {
            storage = LE_SMS_STORAGE_NV;
        }
        break;

        case PA_SMS_STORAGE_SIM:
        {
            storage = LE_SMS_STORAGE_SIM;
        }
        break;

        default:
        {
            LE_ERROR("new message doesn't content Storage area indication");
        }
    }

    // Notify all the registered client's handlers with own reference.
    le_event_Report(StorageStatusEventId, (void*)&storage, sizeof(le_sms_Storage_t));

    LE_DEBUG("All the registered client's handlers notified");
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the transport layer protocol
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetProtocol
(
    pa_sms_Protocol_t* protocolPtr
)
{
    le_mrc_Rat_t rat;
    if (LE_OK != le_mrc_GetRadioAccessTechInUse(&rat))
    {
        LE_ERROR("Could not retrieve the Radio Access Technology");
        return LE_FAULT;
    }

    if (LE_MRC_RAT_CDMA == rat)
    {
        *protocolPtr = PA_SMS_PROTOCOL_CDMA;
    }
    else
    {
        if (LE_MRC_RAT_LTE == rat)
        {
            /* TODO:
             * It is a workaround for LTE Sprint network (temporary solution).
             * LTE Sprint Network "310 120" SMS service center doesn't support 3GPP SMS pdu
             * format. So Home PLMN needs to be checked.
             */
            char mcc[LE_MRC_MCC_BYTES];
            char mnc[LE_MRC_MNC_BYTES];

            if (LE_OK == pa_sim_GetHomeNetworkMccMnc(mcc,
                                             LE_MRC_MCC_BYTES,
                                             mnc,
                                             LE_MRC_MNC_BYTES))
            {
                if ((0 == strncmp(mcc, "310", strlen("310"))) &&
                    (0 == strncmp(mnc, "120", strlen("120"))))
                {
                    *protocolPtr = PA_SMS_PROTOCOL_CDMA;
                }
            }
            else
            {
                LE_ERROR("Could not retrieve MCC/MNC");
            }
            *protocolPtr = PA_SMS_PROTOCOL_GSM;
        }
        else
        {
            *protocolPtr = PA_SMS_PROTOCOL_GSM;
        }
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check data validity and encode PDU message.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CheckAndEncodeMessage
(
    le_sms_Msg_t* msgPtr
)
{
    le_result_t result = LE_OK;

    // Validate data
    switch (msgPtr->format)
    {
        case LE_SMS_FORMAT_TEXT:
            if ((0 == msgPtr->userdataLen) || ('\0' == msgPtr->text[0]))
            {
                LE_ERROR("Text content is invalid for Message Object %p", msgPtr);
                return LE_FORMAT_ERROR;
            }
            break;

        case LE_SMS_FORMAT_BINARY:
            if (0 == msgPtr->userdataLen)
            {
                LE_ERROR("Binary content is empty for Message Object %p", msgPtr);
                return LE_FORMAT_ERROR;
            }
            break;
        case LE_SMS_FORMAT_UCS2:
            if (0 == msgPtr->userdataLen)
            {
                LE_ERROR("UCS2 content is empty for Message Object %p", msgPtr);
                return LE_FORMAT_ERROR;
            }
            break;

        case LE_SMS_FORMAT_PDU:
            if (0 == msgPtr->pdu.dataLen)
            {
                LE_ERROR("No PDU content for Message Object %p", msgPtr);
                return LE_FORMAT_ERROR;
            }
            break;

        default:
            LE_ERROR("Format %d for Message Object %p is incorrect", msgPtr->format, msgPtr);
            return LE_FORMAT_ERROR;
    }

    if ((msgPtr->format != LE_SMS_FORMAT_PDU) && ('\0' == msgPtr->tel[0]))
    {
        LE_ERROR("Telephone number is invalid for Message Object %p", msgPtr);
        return LE_FORMAT_ERROR;
    }

    // Get transport layer protocol
    if (LE_OK != GetProtocol(&msgPtr->protocol))
    {
        return LE_FAULT;
    }

    // Encode data
    if (!msgPtr->pduReady)
    {
        result = EncodeMessageToPdu(msgPtr);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send connection state event.
 */
//--------------------------------------------------------------------------------------------------
static void SendSmsSendingStateEvent
(
    le_sms_MsgRef_t messageRef
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, messageRef);
    if (NULL == msgPtr)
    {
        LE_ERROR("Message Null");
        return;
    }

    le_sms_CallbackResultFunc_t Myfunction = msgPtr->callBackPtr;

    // Check if a callback function is available.
    if (Myfunction)
    {
        LE_DEBUG("Sending CallBack (%p) Message (%p), Status %d",
                 Myfunction, messageRef, msgPtr->pdu.status);

        // Update sent message count if necessary
        if ((MessageStats.counting) && (LE_SMS_SENT == msgPtr->pdu.status))
        {
            SetMessageCount(LE_SMS_TYPE_TX, MessageStats.txCount + 1);
        }

        Myfunction(messageRef, msgPtr->pdu.status, msgPtr->ctxPtr);
    }
    else
    {
        LE_WARN("No CallBackFunction Found fot message %p, status %d!!",
                messageRef, msgPtr->pdu.status);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function send a message in asynchrone mode.
 *
 * It verifies first if the parameters are valid, then it checks that the modem state can support
 * message sending.
 *
 * @return
 *  - LE_FAULT         The function failed to send the message.
 *  - LE_OK            The function succeeded.
 *  - LE_FORMAT_ERROR  The message content is invalid.
 *  - LE_BAD_PARAMETER Invalid reference provided.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SendAsyncSMS
(
    le_sms_MsgRef_t    msgRef,         ///< [IN] The message(s) to send.
    void * callBack,
    void * context
)
{
    le_result_t   result = LE_FAULT;
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);
    if (NULL == msgPtr)
    {
        LE_ERROR("Invalid reference (%p) provided!", msgRef);
        return LE_BAD_PARAMETER;
    }

    result = CheckAndEncodeMessage(msgPtr);

    /* Send */
    if (result == LE_OK)
    {
        CmdRequest_t msgCommand;
        // Save the client session "msgSession" associated with the request reference "reqRef".
        msgPtr->sessionRef = le_sms_GetClientSessionRef();

        msgPtr->pdu.status = LE_SMS_SENDING;

        // Sending Message.
        msgCommand.command = LE_SMS_CMD_TYPE_SEND;
        msgCommand.msgRef = msgRef;
        msgPtr->callBackPtr = callBack;
        msgPtr->ctxPtr = context;

        LE_INFO("Send Send command for message (%p)", msgRef);
        le_event_Report(SmsCommandEventId, &msgCommand, sizeof(msgCommand));
    }
    else
    {
        LE_ERROR("Cannot encode Message Object %p", msgPtr);
        result = LE_FORMAT_ERROR;
    }

    return result;
}



//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a command.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessSmsSendingCommandHandler
(
    void* msgCommand
)
{
    le_result_t res;
    uint32_t command = ((CmdRequest_t*)msgCommand)->command;
    le_sms_MsgRef_t messageRef = ((CmdRequest_t*)msgCommand)->msgRef;

    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, messageRef);

    if (NULL == msgPtr)
    {
        LE_DEBUG("No more message reference (%p) valid", messageRef);
        return;
    }

    switch (command)
    {
        case LE_SMS_CMD_TYPE_SEND:
        {
            le_sem_Wait(SmsSem);
            LE_INFO("LE_SMS_CMD_TYPE_SEND message (%p) ", messageRef);

            res = pa_sms_SendPduMsg(msgPtr->protocol, msgPtr->pdu.dataLen, msgPtr->pdu.data,
                                    &msgPtr->messageReference, PA_SMS_SENDING_TIMEOUT,
                                    &msgPtr->pdu.errorCode);
            if (LE_OK == res)
            {
                msgPtr->pdu.status = LE_SMS_SENT;
            }
            else if (LE_TIMEOUT == res)
            {
                msgPtr->pdu.status = LE_SMS_SENDING_TIMEOUT;
            }
            else
            {
                msgPtr->pdu.status = LE_SMS_SENDING_FAILED;
            }
            LE_INFO("Async send command status: %d", msgPtr->pdu.status);
            le_sem_Post(SmsSem);
            SendSmsSendingStateEvent(messageRef);
        }
        break;

        default:
        {
            LE_ERROR("Command %i is not valid", command);
        }
        break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This thread does the actual work of Pool and send a SMS.
 */
//--------------------------------------------------------------------------------------------------
static void* SmsSenderThread
(
    void* contextPtr
)
{
    LE_INFO("Sms command Thread started");

    // Connect to services used by this thread
    le_cfg_ConnectService();

    // Register for SMS command events.
    le_event_AddHandler("ProcessCommandHandler", SmsCommandEventId,
        ProcessSmsSendingCommandHandler);

    le_sem_Post(SmsSem);

    // Watchdog SMS event loop
    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_MonitorEventLoop(MS_WDOG_SMS_LOOP, watchdogInterval);

    // Run the event loop
    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function to the close session service.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] Session reference of client application.
    void*               contextPtr   ///< [IN] Context pointer of CloseSessionEventHandler.
)
{
    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    // Clean session context.
    LE_ERROR("SessionRef (%p) has been closed", sessionRef);

    // Get session context.
    SessionCtxNode_t* sessionCtxPtr = GetSessionCtx(sessionRef);

    if (sessionCtxPtr)
    {
        // Peek the head node of message reference list without removing the node from the list.
        le_dls_Link_t* linkPtr = le_dls_Peek(&(sessionCtxPtr->msgRefList));

        while (linkPtr)
        {
            MsgRefNode_t* msgRefPtr = CONTAINER_OF(linkPtr, MsgRefNode_t, link);

            // Get the next node from message reference list.
            linkPtr = le_dls_PeekNext(&(sessionCtxPtr->msgRefList), linkPtr);

            // Delete the message data structure,
            // Delete the session context if no more message data
            le_sms_Delete(msgRefPtr->msgRef);
        }
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(ListRefMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        le_sms_List_t* smsListPtr = (le_sms_List_t*)le_ref_GetValue(iterRef);

        // Check if the session reference saved matchs with the current session reference.
        if (smsListPtr->sessionRef == sessionRef)
        {
            le_sms_MsgListRef_t msgListRef = (le_sms_MsgListRef_t) le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release message reference 0x%p, sessionRef 0x%p", msgListRef, sessionRef);
            // Release message List.
            le_sms_DeleteList(msgListRef);
        }
        // Get the next value in the reference map.
        result = le_ref_NextNode(iterRef);
    }
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the SMS operations component.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_Init
(
    void
)
{
    // Initialize the smsPdu module.
    smsPdu_Initialize();

    // Initialize the message statistics
    InitializeMessageStatistics();

    // Initialize Status Report activation state
    StatusReportActivation = GetStatusReportState();

    // Create a pool for Message objects.
    MsgPool = le_mem_CreatePool("SmsMsgPool", sizeof(le_sms_Msg_t));
    le_mem_ExpandPool(MsgPool, MAX_NUM_OF_SMS_MSG);

    // Create the Safe Reference Map to use for Message object Safe References.
    MsgRefMap = le_ref_CreateMap("SmsMsgMap", MAX_NUM_OF_SMS_MSG);

    // Create a pool for List objects.
    ListPool = le_mem_CreatePool("ListSmsPool", sizeof(le_sms_List_t));
    le_mem_ExpandPool(ListPool, MAX_NUM_OF_LIST);

    // Create the Safe Reference Map to use for List object Safe References.
    ListRefMap = le_ref_CreateMap("ListSmsMap", MAX_NUM_OF_LIST);

    // Create a pool for Message references list.
    ReferencePool = le_mem_CreatePool("SmsReferencePool", sizeof(le_sms_MsgReference_t));
    le_mem_ExpandPool(ReferencePool, MAX_NUM_OF_SMS_MSG);

    MsgRefPool = le_mem_CreatePool("MsgRefPool", sizeof(MsgRefNode_t));
    le_mem_ExpandPool(MsgRefPool, SMS_MAX_SESSION*MAX_NUM_OF_SMS_MSG);

    // Create pool for received message handler.
    HandlerPool = le_mem_CreatePool("HandlerPool", sizeof(HandlerCtxNode_t));
    le_mem_ExpandPool(HandlerPool, SMS_MAX_SESSION);

    // Create safe reference map to use handler object safe references.
    HandlerRefMap = le_ref_CreateMap("HandlerRefMap", SMS_MAX_SESSION);

    // Create pool for client session list.
    SessionCtxPool = le_mem_CreatePool("SessionCtxPool", sizeof(SessionCtxNode_t));
    le_mem_ExpandPool(SessionCtxPool, SMS_MAX_SESSION);

    // Create an event Id for SMS storage indication.
    StorageStatusEventId = le_event_CreateId("StorageStatusEventId", sizeof(le_sms_Storage_t));

    // Add a handler to the close session service.
    le_msg_AddServiceCloseHandler(le_sms_GetServiceRef(), CloseSessionEventHandler, NULL);

    SessionCtxList = LE_DLS_LIST_INIT;

    // Register a handler function for SMS storage status indication.
    if (pa_sms_AddStorageStatusHandler(StorageIndicationHandler) == NULL)
    {
        LE_WARN("failed to register a handler function for SMS storage");
    }

    SmsSem = le_sem_Create("SmsSem", 1);

    // Init the SMS command Event Id.
    SmsCommandEventId = le_event_CreateId("SmsSendCmd", sizeof(CmdRequest_t));
    le_thread_Start(le_thread_Create(WDOG_THREAD_NAME_SMS_COMMAND_SENDING, SmsSenderThread, NULL));

    le_sem_Wait(SmsSem);

    // Register a handler function for new message indication.
    if (pa_sms_SetNewMsgHandler(NewSmsHandler) != LE_OK)
    {
        LE_CRIT("Add pa_sms_SetNewMsgHandler failed");
        return LE_FAULT;
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create an SMS Message data structure.
 *
 * @return A reference to the new Message object.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_Create
(
    void
)
{
    le_sms_Msg_t  *msgPtr;

    SessionCtxNode_t* sessionCtxPtr = GetSessionCtx(le_sms_GetClientSessionRef());

    if (!sessionCtxPtr)
    {
        // Create the session context
        sessionCtxPtr = CreateSessionCtx();

        if (!sessionCtxPtr)
        {
            LE_ERROR("Impossible to create the session context");
            return NULL;
        }
    }

    // Create the message node.
    msgPtr = (le_sms_Msg_t*)le_mem_ForceAlloc(MsgPool);

    msgPtr->timestamp[0] = '\0';
    msgPtr->tel[0] = '\0';
    msgPtr->text[0] = '\0';
    msgPtr->userdataLen = 0;
    msgPtr->pduReady = false;
    msgPtr->pdu.status = LE_SMS_UNSENT;
    msgPtr->pdu.dataLen = 0;
    msgPtr->pdu.errorCode.code3GPP2 = LE_SMS_ERROR_3GPP2_MAX;
    msgPtr->pdu.errorCode.rp = LE_SMS_ERROR_3GPP_MAX;
    msgPtr->pdu.errorCode.tp = LE_SMS_ERROR_3GPP_MAX;
    msgPtr->pdu.errorCode.platformSpecific = 0;
    msgPtr->readonly = false;
    msgPtr->inAList = false;
    msgPtr->smsUserCount = 1;
    msgPtr->delAsked = false;
    msgPtr->type = LE_SMS_TYPE_TX;
    msgPtr->messageId = 0;
    msgPtr->messageSerialNumber = 0;
    msgPtr->callBackPtr = NULL;
    msgPtr->ctxPtr = NULL;
    msgPtr->format = LE_SMS_FORMAT_UNKNOWN;
    msgPtr->messageReference = 0;
    msgPtr->typeOfAddress = 0;
    msgPtr->dischargeTime[0] = '\0';
    msgPtr->status = 0;

    // Return a Safe Reference for this message object.
    return SetMsgRefForSessionCtx(msgPtr, sessionCtxPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the timeout to send a SMS Message.
 *
 * @return
 * - LE_FAULT         Message is not in UNSENT state or is Read-Only.
 * - LE_OK            Function succeeded.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 *
 * @deprecated
 *      This API should not be used for new applications and will be removed in a future version
 *      of Legato.
 */
 //--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetTimeout
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    uint32_t timeout
        ///< [IN]
        ///< Timeout in seconds.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }

    if(msgPtr->readonly)
    {
        LE_ERROR("Message is Read-only");
        return LE_FAULT;
    }

    if(msgPtr->pdu.status != LE_SMS_UNSENT)
    {
        LE_ERROR("Message is not in UNSENT state");
        return LE_FAULT;
    }

    if (timeout == 0)
    {
        LE_ERROR("Timeout is equal to zero");
        return LE_FAULT;
    }

    LE_WARN("Deprecated API, should not be used anymore");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete a Message data structure.
 *
 * It deletes the Message data structure, all the allocated memory is freed. However if several
 * Users own the Message object (for example in the case of several handler functions registered for
 * SMS message reception) the Message object will be actually deleted only if it remains one User
 * owning the Message object.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_Delete
(
    le_sms_MsgRef_t  msgRef    ///< [IN] The pointer to the message data structure.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (NULL == msgPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return;
    }

    if (msgPtr->inAList)
    {
        LE_KILL_CLIENT("This message (ref.%p) belongs to a Rx List ! Call 'DeleteList' instead.",
                        msgRef);
        return;
    }

    // Invalidate the Safe Reference.
    LE_DEBUG("le_sms_Delete obj[%p], ref[%p], Delete %c, cpt users = %d", msgPtr, msgRef,
        (msgPtr->delAsked ? 'Y' : 'N'), msgPtr->smsUserCount);
    if ((msgPtr->delAsked) && (1 == msgPtr->smsUserCount))
    {
        le_sms_DeleteFromStorage(msgRef);
    }
    (msgPtr->smsUserCount)--;

    SessionCtxNode_t* sessionCtxPtr = GetSessionCtxFromMsgRef(msgRef);
    if (!sessionCtxPtr)
    {
        LE_ERROR("No sessionCtx found for msgRef %p !!!", msgRef);
        return;
    }

    // Remove the msgRef from the sessionCtx.
    RemoveMsgRefFromSessionCtx(sessionCtxPtr, msgRef);

    if (0 == msgPtr->smsUserCount)
    {
        msgPtr->callBackPtr = NULL;

        // release the message object.
        le_mem_Release(msgPtr);
    }
    else
    {
        LE_DEBUG("smsUserCount is not reached 0");
    }

    if ((le_dls_NumLinks(&(sessionCtxPtr->handlerList)) == 0) &&
             (le_dls_NumLinks(&(sessionCtxPtr->msgRefList)) == 0))
    {
        // delete the session context as it is not used anymore
        le_dls_Remove(&SessionCtxList, &(sessionCtxPtr->link));
        le_mem_Release(sessionCtxPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the message format.
 *
 * @return The message format.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Format_t le_sms_GetFormat
(
    le_sms_MsgRef_t    msgRef   ///< [IN] The pointer to the message data structure.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_SMS_FORMAT_UNKNOWN;
    }

    return (msgPtr->format);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the message type.
 *
 * @return
 *  - Message type.
 *  - LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Type_t le_sms_GetType
(
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Reference to the message object.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (NULL == msgPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_BAD_PARAMETER;
    }

    return (msgPtr->type);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Cell Broadcast Message Identifier.
 *
 * @return
 * - LE_FAULT       Message is not a cell broadcast type.
 * - LE_OK          Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetCellBroadcastId
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    uint16_t* messageIdPtr
        ///< [OUT]
        ///< Cell Broadcast Message Identifier.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_FAULT;
    }

    if (messageIdPtr == NULL)
    {
        LE_KILL_CLIENT("messageIdPtr is NULL");
        return LE_FAULT;
    }

    if (msgPtr->type != LE_SMS_TYPE_BROADCAST_RX)
    {
        LE_ERROR("It is not a Cell Broadcast Message");
        return LE_FAULT;
    }

    *messageIdPtr = msgPtr->messageId;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Cell Broadcast Message Serial Number.
 *
 * @return
 * - LE_FAULT       Message is not a cell broadcast type.
 * - LE_OK          Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetCellBroadcastSerialNumber
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    uint16_t* serialNumberPtr
        ///< [OUT]
        ///< Cell Broadcast Serial Number.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_FAULT;
    }

    if (serialNumberPtr == NULL)
    {
        LE_KILL_CLIENT("serialNumberPtr is NULL");
        return LE_FAULT;
    }

    if (msgPtr->type != LE_SMS_TYPE_BROADCAST_RX)
    {
        LE_ERROR("It is not a Cell Broadcast Message");
        return LE_FAULT;
    }

    *serialNumberPtr = msgPtr->messageSerialNumber;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the Telephone destination number.
 *
 * The Telephone number is defined in ITU-T recommendations E.164/E.163.
 * E.164 numbers can have a maximum of fifteen digits and are usually written with a '+' prefix.
 *
 * @return LE_NOT_PERMITTED The message is Read-Only.
 * @return LE_BAD_PARAMETER The Telephone destination number length is equal to zero.
 * @return LE_OK            The function succeeded.
 *
 * @note If telephone destination number is too long is too long (max LE_MDMDEFS_PHONE_NUM_MAX_LEN
 *       digits), it is a fatal error, the function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetDestination
(
    le_sms_MsgRef_t   msgRef,  ///< [IN] The pointer to the message data structure.
    const char*       destPtr  ///< [IN] The telephone number string.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }

    size_t length = strnlen(destPtr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);

    if(length > (LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1))
    {
        LE_KILL_CLIENT("strlen(dest) > %d", (LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1));
        return LE_FAULT;
    }

    if(msgPtr->readonly)
    {
        return LE_NOT_PERMITTED;
    }

    if(length == 0)
    {
        return LE_BAD_PARAMETER;
    }

    msgPtr->pduReady = false; // PDU must be regenerated

    le_utf8_Copy(msgPtr->tel, destPtr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES, NULL);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Sender Telephone number.
 *
 * The output parameter is updated with the Telephone number. If the Telephone number string exceed
 * the value of 'len' parameter, a LE_OVERFLOW error code is returned and 'tel' is filled until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'tel'.
 *
 * @return LE_NOT_PERMITTED The message is not a received message.
 * @return LE_OVERFLOW      The Telephone number length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetSenderTel
(
    le_sms_MsgRef_t  msgRef,  ///< [IN] The pointer to the message data structure.
    char*            telPtr,  ///< [OUT] The telephone number string.
    size_t           len      ///< [IN] The length of telephone number string.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }
    if (telPtr == NULL)
    {
        LE_KILL_CLIENT("telPtr is NULL !");
        return LE_FAULT;
    }

    /* Only received messages have a sender */
    switch (msgPtr->pdu.status)
    {
        case LE_SMS_RX_READ:
        case LE_SMS_RX_UNREAD:
            break;
        default:
            LE_ERROR("Error.%d : It is not a received message", LE_NOT_PERMITTED);
            return LE_NOT_PERMITTED;
    }

    if (strlen(msgPtr->tel) > (len - 1))
    {
        return LE_OVERFLOW;
    }
    else
    {
        strncpy(telPtr, msgPtr->tel, len);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Service Center Time Stamp string.
 *
 * The output parameter is updated with the Time Stamp string. If the Time Stamp string exceed the
 * value of 'len' parameter, a LE_OVERFLOW error code is returned and 'timestamp' is filled until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'timestamp'.
 *
 * @return LE_NOT_PERMITTED The message is not a received message.
 * @return LE_OVERFLOW      The Timestamp number length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTimeStamp
(
    le_sms_MsgRef_t  msgRef,       ///< [IN] The pointer to the message data structure.
    char*            timestampPtr, ///< [OUT] The message time stamp (in text mode).
    ///        string format: "yy/MM/dd,hh:mm:ss+/-zz"
    ///        (Year/Month/Day,Hour:Min:Seconds+/-TimeZone)
    /// The time zone indicates the difference, expressed in quarters of an hours between the
    ///  local time and GMT.
    size_t           len           ///< [IN] The length of timestamp string.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }
    if (timestampPtr == NULL)
    {
        LE_KILL_CLIENT("timestampPtr is NULL !");
        return LE_FAULT;
    }

    // Not available for Cell Broadcast.
    if(msgPtr->protocol == PA_SMS_PROTOCOL_GW_CB)
    {
        return LE_NOT_PERMITTED;
    }

    // Only received messages are read only.
    if(!msgPtr->readonly)
    {
        LE_ERROR("Error.%d : It is not a received message", LE_NOT_PERMITTED);
        return LE_NOT_PERMITTED;
    }

    if (strlen(msgPtr->timestamp) > (len - 1))
    {
        return LE_OVERFLOW;
    }
    else
    {
        strncpy(timestampPtr, msgPtr->timestamp, len);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the message Length value.
 *
 * @return The number of characters for text messages, or the length of the data in bytes for raw
 *         binary messages.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
size_t le_sms_GetUserdataLen
(
    le_sms_MsgRef_t msgRef ///< [IN] The pointer to the message data structure.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return 0;
    }

    switch (msgPtr->format)
    {
        case LE_SMS_FORMAT_TEXT:
        case LE_SMS_FORMAT_BINARY:
            return msgPtr->userdataLen;
        case LE_SMS_FORMAT_UCS2:
            return (msgPtr->userdataLen / 2);
        default:
            return 0;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the message Length value.
 *
 * @return The length of the data in bytes of the PDU message.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
size_t le_sms_GetPDULen
(
    le_sms_MsgRef_t msgRef ///< [IN] The pointer to the message data structure.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return 0;
    }

    if ((msgPtr->readonly == false) && (!msgPtr->pduReady))
    {
        EncodeMessageToPdu(msgPtr);
    }

    if (msgPtr->pduReady)
    {
        return (msgPtr->pdu.dataLen);
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the UCS2 Message (16-bit format).
 *
 * Output parameters are updated with the UCS2 message content and the number of characters. If
 * the UCS2 data exceed the value of the length input parameter, a LE_OVERFLOW error
 * code is returned and 'ucs2Ptr' is filled until of the number of chars specified.
 *
 * @return
 *  - LE_FORMAT_ERROR  Message is not in binary format.
 *  - LE_OVERFLOW      Message length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetUCS2
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    uint16_t* ucs2Ptr,
        ///< [OUT]
        ///< UCS2 message.

    size_t* ucs2NumElementsPtr
        ///< [INOUT]
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }

    if (ucs2Ptr == NULL)
    {
        LE_KILL_CLIENT("ucs2Ptr is NULL !");
        return LE_FAULT;
    }

    if (ucs2NumElementsPtr == NULL)
    {
        LE_KILL_CLIENT("ucs2NumElementsPtr is NULL !");
        return LE_FAULT;
    }

    if (msgPtr->format != LE_SMS_FORMAT_UCS2)
    {
        LE_ERROR("Error.%d : Invalid format!", LE_FORMAT_ERROR);
        return LE_FORMAT_ERROR;
    }

    if (msgPtr->userdataLen > (*ucs2NumElementsPtr*2))
    {
        memcpy((uint8_t *) ucs2Ptr, msgPtr->binary, (*ucs2NumElementsPtr*2));
        LE_ERROR("datalen %d > Buff size %d",(int)msgPtr->userdataLen,(int)(*ucs2NumElementsPtr*2));
        return LE_OVERFLOW;
    }
    else
    {
        memcpy ((uint8_t *) ucs2Ptr, msgPtr->binary, msgPtr->userdataLen);
        *ucs2NumElementsPtr = msgPtr->userdataLen / 2;
        return LE_OK;
    }

    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create and asynchronously send a text message.
 *
 * @return
 *  - le_sms_Msg Reference to the new Message object pooled.
 *  - NULL Not possible to pool a new message.
 *
 * @note If telephone destination number is too long is too long (max LE_MDMDEFS_PHONE_NUM_MAX_LEN
 *       digits), it is a fatal error, the function will not return.
 * @note If message is too long (max LE_SMS_TEXT_MAX_LEN digits), it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_SendText
(
    const char* destStr,
    ///< [IN]
    ///< Telephone number string.

    const char* textStr,
    ///< [IN]
    ///< SMS text.

    le_sms_CallbackResultFunc_t handlerPtr,
    ///< [IN]

    void* contextPtr
    ///< [IN]
)
{
    le_result_t res = LE_FAULT;
    le_sms_MsgRef_t messageRef = NULL;

    messageRef = le_sms_Create();
    LE_DEBUG("New message ref (%p) created",messageRef);

    res = le_sms_SetDestination(messageRef, destStr);
    if (res != LE_OK)
    {
        le_sms_Delete(messageRef);
        LE_ERROR("Failed to set destination!");
        return NULL;
    }

    res = le_sms_SetText(messageRef, textStr);
    if (res != LE_OK)
    {
        le_sms_Delete(messageRef);
        LE_ERROR("Failed to set text !");
        return NULL;
    }

    res = SendAsyncSMS(messageRef, handlerPtr, contextPtr);
    if (res != LE_OK)
    {
        le_sms_Delete(messageRef);
        LE_ERROR("Failed to pool new sms for sending (%p)", messageRef);
        return NULL;
    }

    LE_DEBUG("New message ref (%p) pooled",messageRef);
    return messageRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create and asynchronously send a PDU message.
 *
 * @return
 *  - le_sms_Msg Reference to the new Message object pooled.
 *  - NULL Not possible to pool a new message.
 *
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_SendPdu
(
    const uint8_t* pduPtr,
    ///< [IN]
    ///< PDU message.

    size_t pduNumElements,
    ///< [IN]

    le_sms_CallbackResultFunc_t handlerPtr,
    ///< [IN]

    void* contextPtr
    ///< [IN]
)
{
    le_result_t res = LE_FAULT;
    le_sms_MsgRef_t messageRef = NULL;

    if (pduPtr == NULL)
    {
        LE_KILL_CLIENT("pduPtr is NULL !");
        return NULL;
    }

    messageRef = le_sms_Create();

    res = le_sms_SetPDU(messageRef, pduPtr, pduNumElements);
    if (res != LE_OK)
    {
        le_sms_Delete(messageRef);
        LE_ERROR("Failed to set pdu !");
        return NULL;
    }

    res = SendAsyncSMS(messageRef, handlerPtr, contextPtr);
    if (res != LE_OK)
    {
        le_sms_Delete(messageRef);
        LE_ERROR("Failed to pool new sms for sending");
        return NULL;
    }

    return messageRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the Text Message content.
 *
 * @return LE_NOT_PERMITTED The message is Read-Only.
 * @return LE_BAD_PARAMETER The text message length is equal to zero.
 * @return LE_OK            The function succeeded.
 *
 * @note Text Message is encoded in ASCII format (ISO8859-15) and characters have to exist in
 *  the GSM 23.038 7 bit alphabet.
 *
 * @note If message is too long (max LE_SMS_TEXT_MAX_LEN digits), it is a fatal error, the
 *       function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetText
(
    le_sms_MsgRef_t       msgRef, ///< [IN] The pointer to the message data structure.
    const char*           textPtr ///< [IN] The SMS text.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }

    if(msgPtr->readonly)
    {
        return LE_NOT_PERMITTED;
    }

    size_t length = strnlen(textPtr, LE_SMS_TEXT_MAX_BYTES);

    if(length > (LE_SMS_TEXT_MAX_BYTES-1))
    {
        LE_KILL_CLIENT("strlen(text) > %d", (LE_SMS_TEXT_MAX_BYTES-1));
        return LE_FAULT;
    }
    else if (length == 0)
    {
        return LE_BAD_PARAMETER;
    }

    msgPtr->format = LE_SMS_FORMAT_TEXT;
    msgPtr->userdataLen = length;
    msgPtr->pduReady = false;
    LE_DEBUG("Try to copy data %s, len.%zd @ msgPtr->text.%p for msgPtr.%p",
             textPtr, length, msgPtr->text, msgPtr);

    le_utf8_Copy(msgPtr->text, textPtr, sizeof(msgPtr->text), NULL);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the binary message content.
 *
 * @return LE_NOT_PERMITTED The message is Read-Only.
 * @return LE_BAD_PARAMETER The length of the data is equal to zero.
 * @return LE_OK            The function succeeded.
 *
 * @note If length of the data is too long (max LE_SMS_BINARY_MAX_BYTES bytes), it is a fatal
 *       error, the function will not return.

 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetBinary
(
    le_sms_MsgRef_t   msgRef, ///< [IN] The pointer to the message data structure.
    const uint8_t*    binPtr, ///< [IN] The binary data.
    size_t            len     ///< [IN] The length of the data in bytes.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }
    if (binPtr == NULL)
    {
        LE_KILL_CLIENT("binPtr is NULL !");
        return LE_FAULT;
    }

    if(msgPtr->readonly)
    {
        return LE_NOT_PERMITTED;
    }

    if(len==0)
    {
        return LE_BAD_PARAMETER;
    }

    if(len > LE_SMS_BINARY_MAX_BYTES)
    {
        LE_KILL_CLIENT("len > %d", LE_SMS_BINARY_MAX_BYTES);
        return LE_FAULT;
    }

    msgPtr->format = LE_SMS_FORMAT_BINARY;
    msgPtr->userdataLen = len;

    memcpy(msgPtr->binary, binPtr, len);

    LE_DEBUG("copy data, len.%zd @ msgPtr->userdata.%p for msgPtr.%p", len, msgPtr->binary, msgPtr);

    msgPtr->pduReady = false;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the PDU message content.
 *
 * @return LE_NOT_PERMITTED The message is Read-Only.
 * @return LE_BAD_PARAMETER The length of the data is equal to zero.
 * @return LE_OK            The function succeeded.
 *
 * @note If length of the data is too long (max LE_SMS_PDU_MAX_BYTES bytes), it is a fatal error,
 *       the function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetPDU
(
    le_sms_MsgRef_t   msgRef, ///< [IN] The pointer to the message data structure.
    const uint8_t*    pduPtr, ///< [IN] The PDU message.
    size_t            len     ///< [IN] The length of the data in bytes.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }
    if (pduPtr == NULL)
    {
        LE_KILL_CLIENT("pduPtr is NULL !");
        return LE_FAULT;
    }

    if(msgPtr->readonly)
    {
        return LE_NOT_PERMITTED;
    }

    if(len==0)
    {
        return LE_BAD_PARAMETER;
    }

    if(len > LE_SMS_PDU_MAX_BYTES)
    {
        LE_KILL_CLIENT("len > %d", LE_SMS_PDU_MAX_BYTES);
        return LE_FAULT;
    }

    msgPtr->format = LE_SMS_FORMAT_PDU;
    msgPtr->pdu.dataLen = len;

    memcpy(msgPtr->pdu.data, pduPtr, len);

    LE_DEBUG("copy data, len.%zd @ msgPtr->pdu.%p for msgPtr.%p", len, msgPtr->pdu.data, msgPtr);

    msgPtr->pduReady = true;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the UCS2 message content (16 bit format).
 *
 * @return
 *  - LE_NOT_PERMITTED Message is Read-Only.
 *  - LE_BAD_PARAMETER Length of the data is equal to zero.
 *  - LE_OK            Function succeeded.
 *
 * @note If length of the data is too long (max LE_SMS_UCS2_MAX_CHARS), it is a fatal
 *       error, the function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetUCS2
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    const uint16_t* ucs2Ptr,
        ///< [IN]
        ///< UCS2 message.

    size_t ucs2NumElements
        ///< [IN]
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }
    if (ucs2Ptr == NULL)
    {
        LE_KILL_CLIENT("ucs2Ptr is NULL !");
        return LE_FAULT;
    }

    if(msgPtr->readonly)
    {
        LE_ERROR("readonly");
        return LE_NOT_PERMITTED;
    }

    if(ucs2NumElements == 0)
    {
        LE_ERROR("ucs2NumElements empty");
        return LE_BAD_PARAMETER;
    }

    if(ucs2NumElements > LE_SMS_UCS2_MAX_CHARS)
    {
        LE_KILL_CLIENT("ucs2NumElements > %d", LE_SMS_UCS2_MAX_CHARS);
        return LE_FAULT;
    }

    msgPtr->format = LE_SMS_FORMAT_UCS2;
    msgPtr->userdataLen = ucs2NumElements*2;

    memcpy(msgPtr->binary, (uint8_t *) ucs2Ptr, msgPtr->userdataLen);

    LE_DEBUG("copy data, ucs2NumElements.%zd @ msgPtr->userdata.%p for ucs2Ptr.%p",
        ucs2NumElements, msgPtr->binary, ucs2Ptr);

    msgPtr->pduReady = false;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the text Message.
 *
 * Output parameter is updated with the text string encoded in ASCII format. If the text string
 * exceeds the value of 'len' parameter, LE_OVERFLOW error code is returned and 'text' is filled
 * until 'len-1' characters and a null-character is implicitly appended at the end of 'text'.
 *
 * @return LE_OVERFLOW      The message length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetText
(
    le_sms_MsgRef_t  msgRef,  ///< [IN]  The pointer to the message data structure.
    char*            textPtr, ///< [OUT] The SMS text.
    size_t           len      ///< [IN] The maximum length of the text message.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }

    if (textPtr == NULL)
    {
        LE_KILL_CLIENT("textPtr is NULL !");
        return LE_FAULT;
    }

    if (msgPtr->format != LE_SMS_FORMAT_TEXT)
    {
        LE_ERROR("Error.%d : Invalid format!", LE_FORMAT_ERROR);
        return LE_FORMAT_ERROR;
    }

    if (strlen(msgPtr->text) > (len - 1))
    {
        return LE_OVERFLOW;
    }
    else
    {
        strncpy(textPtr, msgPtr->text, len);
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the binary Message.
 *
 * The output parameters are updated with the binary message content and the length of the raw
 * binary message in bytes. If the binary data exceed the value of 'len' input parameter, a
 * LE_OVERFLOW error code is returned and 'raw' is filled until 'len' bytes.
 *
 * @return LE_FORMAT_ERROR  Message is not in binary format.
 * @return LE_OVERFLOW      The message length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetBinary
(
    le_sms_MsgRef_t  msgRef, ///< [IN]  The pointer to the message data structure.
    uint8_t*         binPtr, ///< [OUT] The binary message.
    size_t*          lenPtr  ///< [IN,OUT] The length of the binary message in bytes.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }

    if (binPtr == NULL)
    {
        LE_KILL_CLIENT("binPtr is NULL !");
        return LE_FAULT;
    }

    if (lenPtr == NULL)
    {
        LE_KILL_CLIENT("lenPtr is NULL !");
        return LE_FAULT;
    }

    if (msgPtr->format != LE_SMS_FORMAT_BINARY)
    {
        LE_ERROR("Error.%d : Invalid format!", LE_FORMAT_ERROR);
        return LE_FORMAT_ERROR;
    }

    if (msgPtr->userdataLen > *lenPtr)
    {
        memcpy(binPtr, msgPtr->binary, *lenPtr);
        return LE_OVERFLOW;
    }
    else
    {
        memcpy (binPtr, msgPtr->binary, msgPtr->userdataLen);
        *lenPtr=msgPtr->userdataLen;
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the PDU message.
 *
 * The output parameters are updated with the PDU message content and the length of the PDU message
 * in bytes. If the PDU data exceed the value of 'len' input parameter, a LE_OVERFLOW error code is
 * returned and 'pdu' is filled until 'len' bytes.
 *
 * @return LE_FORMAT_ERROR  Unable to encode the message in PDU (only for outgoing messages).
 * @return LE_OVERFLOW      The message length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetPDU
(
    le_sms_MsgRef_t  msgRef, ///< [IN]  The pointer to the message data structure.
    uint8_t*         pduPtr, ///< [OUT] The PDU message.
    size_t*          lenPtr  ///< [IN,OUT] The length of the PDU message in bytes.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }

    if (pduPtr == NULL)
    {
        LE_KILL_CLIENT("pduPtr is NULL !");
        return LE_FAULT;
    }

    if (lenPtr == NULL)
    {
        LE_KILL_CLIENT("lenPtr is NULL !");
        return LE_FAULT;
    }

    if ((msgPtr->readonly == false)
                    && (msgPtr->protocol != PA_SMS_PROTOCOL_GW_CB)
                    && (!msgPtr->pduReady))
    {
        /* Get transport layer protocol */
        if (LE_OK != GetProtocol(&msgPtr->protocol))
        {
            return LE_FAULT;
        }
        EncodeMessageToPdu(msgPtr);
    }

    if (!msgPtr->pduReady)
    {
        return LE_FORMAT_ERROR;
    }

    if (msgPtr->pdu.dataLen > *lenPtr)
    {
        memcpy(pduPtr, msgPtr->pdu.data, *lenPtr);
        return LE_OVERFLOW;
    }
    else
    {
        memcpy(pduPtr, msgPtr->pdu.data, msgPtr->pdu.dataLen);
        *lenPtr = msgPtr->pdu.dataLen;
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler function for SMS message reception.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_sms_RxMessageHandlerRef_t le_sms_AddRxMessageHandler
(
    le_sms_RxMessageHandlerFunc_t handlerFuncPtr, ///< [IN] The handler function for message
    ///  reception.
    void*                         contextPtr      ///< [IN] The handler's context.
)
{
    if (NULL == handlerFuncPtr)
    {
        LE_KILL_CLIENT("handlerFuncPtr is NULL !");
        return NULL;
    }

    // search the sessionCtx; create it if doesn't exist.
    SessionCtxNode_t* sessionCtxPtr = GetSessionCtx(le_sms_GetClientSessionRef());
    if (!sessionCtxPtr)
    {
        // Create the session context.
        sessionCtxPtr = CreateSessionCtx();
    }

    // Add the handler in the list.
    HandlerCtxNode_t* handlerCtxPtr = le_mem_ForceAlloc(HandlerPool);

    handlerCtxPtr->handlerFuncPtr = handlerFuncPtr;
    handlerCtxPtr->userContext = contextPtr;
    handlerCtxPtr->handlerRef = le_ref_CreateRef(HandlerRefMap, handlerCtxPtr);
    handlerCtxPtr->sessionCtxPtr = sessionCtxPtr;
    handlerCtxPtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&(sessionCtxPtr->handlerList), &(handlerCtxPtr->link));

    return handlerCtxPtr->handlerRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister a handler function.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_RemoveRxMessageHandler
(
    le_sms_RxMessageHandlerRef_t   handlerRef ///< [IN] The handler reference.
)
{
    // Get the hander context
    HandlerCtxNode_t* handlerCtxPtr = le_ref_Lookup(HandlerRefMap, handlerRef);
    if (NULL == handlerCtxPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", handlerRef);
        return;
    }

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(HandlerRefMap, handlerRef);

    SessionCtxNode_t* sessionCtxPtr = handlerCtxPtr->sessionCtxPtr;
    if (!sessionCtxPtr)
    {
        LE_ERROR("No sessionCtxPtr !!!");
        return;
    }

    // Remove the handler node from the session context.
    le_dls_Remove(&(sessionCtxPtr->handlerList), &(handlerCtxPtr->link));
    le_mem_Release(handlerCtxPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler function for SMS full storage
 * message reception.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_sms_FullStorageEventHandlerRef_t le_sms_AddFullStorageEventHandler
(
    le_sms_FullStorageHandlerFunc_t handlerFuncPtr, ///< [IN] The handler function for SMS
                                                      ///  full storage message indication.
    void*                              contextPtr     ///< [IN] The handler's context.
)
{
    le_event_HandlerRef_t handlerRef;

    if(handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("handlerFuncPtr is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("StorageSms",
                    StorageStatusEventId,
                    FirstLayerStorageSmsHandler,
                    (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_sms_FullStorageEventHandlerRef_t)(handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister a handler function.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_RemoveFullStorageEventHandler
(
    le_sms_FullStorageEventHandlerRef_t   handlerRef ///< [IN] The handler reference.
)
{
    // Remove the handler.
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send an SMS message.
 *
 * It verifies first if the parameters are valid, then it checks that the modem state can support
 * message sending.
 *
 * @return LE_FORMAT_ERROR  The message content is invalid.
 * @return LE_FAULT         The function failed to send the message.
 * @return LE_OK            The function succeeded.
 * @return LE_TIMEOUT       Timeout before the complete sending.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_Send
(
    le_sms_MsgRef_t    msgRef         ///< [IN] The message(s) to send.
)
{
    le_result_t   result = LE_OK;
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }

    /* Send */
    if (CheckAndEncodeMessage(msgPtr) == LE_OK)
    {
        LE_DEBUG("Try to send PDU Msg %p, pdu.%p, pduLen.%u with protocol %d",
                        msgPtr, msgPtr->pdu.data, msgPtr->pdu.dataLen, msgPtr->protocol);

        le_sem_Wait(SmsSem);
        result = pa_sms_SendPduMsg(msgPtr->protocol, msgPtr->pdu.dataLen, msgPtr->pdu.data,
                                   &msgPtr->messageReference, PA_SMS_SENDING_TIMEOUT,
                                   &msgPtr->pdu.errorCode);
        le_sem_Post(SmsSem);

        if (result < 0)
        {
            LE_ERROR("Error.%d : Failed to send Message Object %p", result, msgPtr);
            if (result != LE_TIMEOUT)
            {
                result = LE_FAULT;
            }
        }
        else
        {
            msgPtr->pdu.status = LE_SMS_SENT;
            result = LE_OK;

            // Update sent message count if necessary
            if (MessageStats.counting)
            {
                SetMessageCount(LE_SMS_TYPE_TX, MessageStats.txCount + 1);
            }
        }
    }
    else
    {
        LE_ERROR("Cannot encode Message Object %p", msgPtr);
        result = LE_FORMAT_ERROR;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Send an asynchronous SMS message.
 *
 * Verifies first if the parameters are valid, then it checks the modem state can support
 * message sending.
 *
 * @return LE_FORMAT_ERROR  Message content is invalid.
 * @return LE_FAULT         Function failed to send the message.
 * @return LE_OK            Function succeeded.
 * @return LE_TIMEOUT       Timeout before the complete sending.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SendAsync
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    le_sms_CallbackResultFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    le_result_t res;
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }

    if(msgPtr->readonly)
    {
        LE_ERROR("Message is Read-only");
        return LE_FAULT;
    }

    if(msgPtr->pdu.status != LE_SMS_UNSENT)
    {
        LE_ERROR("Message is not in UNSENT state");
        return LE_FAULT;
    }

    res = SendAsyncSMS(msgRef, handlerPtr, contextPtr);
    LE_ERROR_IF(res != LE_OK, "Failed to pool sms for sending (%p)", msgRef);

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the error code when a 3GPP2 message sending has Failed.
 *
 * @return
 *  - The error code
 *  - LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 *
 * @note It is only applicable for 3GPP2 message sending failure, otherwise
 *       LE_SMS_ERROR_3GPP2_MAX is returned.
 */
//--------------------------------------------------------------------------------------------------
le_sms_ErrorCode3GPP2_t le_sms_Get3GPP2ErrorCode
(
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Reference to the message object.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (NULL == msgPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_BAD_PARAMETER;
    }

    return msgPtr->pdu.errorCode.code3GPP2;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio Protocol and the Transfer Protocol error code when a 3GPP message sending has
 * failed.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 *
 * @note It is only applicable for 3GPP message sending failure, otherwise
 *       LE_SMS_ERROR_3GPP_MAX is returned.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_GetErrorCode
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    le_sms_ErrorCode_t* rpCausePtr,
        ///< [OUT]
        ///< Radio Protocol cause code.

    le_sms_ErrorCode_t* tpCausePtr
        ///< [OUT]
        ///< Transfer Protocol cause code.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);
    if (NULL == msgPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return;
    }

    if (msgPtr == msgRef)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return;
    }

    if (NULL == rpCausePtr)
    {
        LE_KILL_CLIENT("rpCausePtr is NULL");
        return;
    }

    if (NULL == tpCausePtr)
    {
        LE_KILL_CLIENT("tpCausePtr is NULL");
        return;
    }

    *rpCausePtr = msgPtr->pdu.errorCode.rp;
    *tpCausePtr = msgPtr->pdu.errorCode.tp;
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the platform specific error code.
 *
 * Refer to @ref platformConstraintsSpecificErrorCodes for platform specific error code description.
 *
 * @return
 *  - The platform specific error code.
 *  - LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_sms_GetPlatformSpecificErrorCode
(
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Reference to the message object.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (NULL == msgPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_BAD_PARAMETER;
    }

    return msgPtr->pdu.errorCode.platformSpecific;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete an SMS message from the storage area.
 *
 * It verifies first if the parameter is valid, then it checks that the modem state can support
 * message deleting.
 *
 * @return LE_FAULT         The function failed to perform the deletion.
 * @return LE_NO_MEMORY     The message is not present in storage area.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_DeleteFromStorage
(
    le_sms_MsgRef_t msgRef   ///< [IN] The message to delete.
)
{
    le_result_t   resp = LE_OK;
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (NULL == msgPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }

    /* Not available for Cell Broadcast */
    if (PA_SMS_PROTOCOL_GW_CB == msgPtr->protocol)
    {
        LE_DEBUG("SMS Cell  Broadcast not stored");
        return LE_NO_MEMORY;
    }

    /* Not available for non-stored messages */
    if ((PA_SMS_STORAGE_NONE == msgPtr->storage) || (PA_SMS_STORAGE_UNKNOWN == msgPtr->storage))
    {
        LE_DEBUG("Cannot delete non-stored message");
        return LE_NO_MEMORY;
    }

    LE_DEBUG("le_sms_DeleteFromStorage obj[%p], ref[%p], cpt = %d", msgPtr, msgRef,
        msgPtr->smsUserCount);

    if (1 == msgPtr->smsUserCount)
    {
        le_sem_Wait(SmsSem);
        resp = pa_sms_DelMsgFromMem(msgPtr->storageIdx, msgPtr->protocol, msgPtr->storage);
        le_sem_Post(SmsSem);

        if ((LE_COMM_ERROR == resp) || (LE_TIMEOUT == resp))
        {
            return LE_NO_MEMORY;
        }
        else
        {
            msgPtr->delAsked = false;
            if (LE_OK != resp)
            {
                resp = LE_FAULT;
            }
            return resp;
        }
    }
    msgPtr->delAsked = true;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create an object's reference of the list of received messages
 * saved in the SMS message storage area.
 *
 * @return
 *      A reference to the List object. Null pointer if no messages have been retrieved.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgListRef_t le_sms_CreateRxMsgList
(
    void
)
{
    le_sms_List_t* storedRxMsgListObjPtr = (le_sms_List_t*)le_mem_ForceAlloc(ListPool);

    storedRxMsgListObjPtr->list = LE_DLS_LIST_INIT;

    if (ListAllReceivedMessages(storedRxMsgListObjPtr) > 0)
    {
        storedRxMsgListObjPtr->currentLink = NULL;

        // Store client session reference.
        storedRxMsgListObjPtr->sessionRef = le_sms_GetClientSessionRef();

        // Create and return a Safe Reference for this List object.
        storedRxMsgListObjPtr->msgListRef = le_ref_CreateRef(ListRefMap, storedRxMsgListObjPtr);
        return storedRxMsgListObjPtr->msgListRef;
    }
    else
    {
        le_mem_Release(storedRxMsgListObjPtr);
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of the Messages retrieved from the message
 * storage.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_DeleteList
(
    le_sms_MsgListRef_t     msgListRef   ///< [IN] The list of the Messages.
)
{
    le_sms_List_t* listPtr = le_ref_Lookup(ListRefMap, msgListRef);

    if (listPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgListRef);
        return;
    }

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(ListRefMap, msgListRef);

    listPtr->currentLink = NULL;
    ReInitializeList ((le_dls_List_t*) &(listPtr->list));
    le_mem_Release(listPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the first Message object reference in the list of messages
 * retrieved with le_sms_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return le_sms_MsgRef_t  The Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_GetFirst
(
    le_sms_MsgListRef_t        msgListRef ///< [IN] The list of the messages.
)
{
    le_sms_MsgReference_t*  nodePtr;
    le_dls_Link_t*          msgLinkPtr;
    le_sms_List_t*      listPtr = le_ref_Lookup(ListRefMap, msgListRef);

    if (listPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgListRef);
        return NULL;
    }

    msgLinkPtr = le_dls_Peek(&(listPtr->list));
    if (msgLinkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(msgLinkPtr, le_sms_MsgReference_t, listLink);
        listPtr->currentLink = msgLinkPtr;
        return nodePtr->msgRef;
    }
    else
    {
        return NULL;
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the next Message object reference in the list of messages
 * retrieved with le_sms_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return le_sms_MsgRef_t  The Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_GetNext
(
    le_sms_MsgListRef_t        msgListRef ///< [IN] The list of the messages.
)
{
    le_sms_MsgReference_t*  nodePtr;
    le_dls_Link_t*          msgLinkPtr;
    le_sms_List_t*      listPtr = le_ref_Lookup(ListRefMap, msgListRef);

    if (listPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgListRef);
        return NULL;
    }

    // Move to the next node.
    msgLinkPtr = le_dls_PeekNext(&(listPtr->list), listPtr->currentLink);
    if (msgLinkPtr != NULL)
    {
        // Get the node from MsgList.
        nodePtr = CONTAINER_OF(msgLinkPtr, le_sms_MsgReference_t, listLink);
        listPtr->currentLink = msgLinkPtr;
        return nodePtr->msgRef;
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to read the Message status (Received Read, Received Unread, Stored
 * Sent, Stored Unsent).
 *
 * @return The status of the message.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Status_t le_sms_GetStatus
(
    le_sms_MsgRef_t      msgRef        ///< [IN] The message.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_SMS_STATUS_UNKNOWN;
    }
    return msgPtr->pdu.status;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to mark a message as 'read'.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_MarkRead
(
    le_sms_MsgRef_t    msgRef         ///< [IN] The message.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return;
    }

    le_sem_Wait(SmsSem);
    if (pa_sms_ChangeMessageStatus(msgPtr->storageIdx, msgPtr->protocol, LE_SMS_RX_READ,
                    msgPtr->storage) == LE_OK)
    {
        msgPtr->pdu.status = LE_SMS_RX_READ;
    }
    le_sem_Post(SmsSem);

}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to mark a message as 'unread'.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_MarkUnread
(
    le_sms_MsgRef_t    msgRef         ///< [IN] The message.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return;
    }

    le_sem_Wait(SmsSem);
    if (pa_sms_ChangeMessageStatus(msgPtr->storageIdx, msgPtr->protocol, LE_SMS_RX_UNREAD,
                    msgPtr->storage) == LE_OK)
    {
        msgPtr->pdu.status = LE_SMS_RX_UNREAD;
    }
    le_sem_Post(SmsSem);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the SMS center address.
 *
 * Output parameter is updated with the SMS Service center address. If the Telephone number string
 *  exceeds the value of 'len' parameter,  LE_OVERFLOW error code is returned and 'tel' is filled
 *  until 'len-1' characters and a null-character is implicitly appended at the end of 'tel'.
 *
 * @return
 *  - LE_FAULT         Service is not available.
 *  - LE_OVERFLOW      Telephone number length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetSmsCenterAddress
(
    char*            telPtr,  ///< [OUT] SMS center address number string.
    size_t           len      ///< [IN] The length of SMS center address number string.
)
{
    char smscMdmStr[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};
    le_result_t res = LE_FAULT;

    le_sem_Wait(SmsSem);
    res = pa_sms_GetSmsc(smscMdmStr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
    le_sem_Post(SmsSem);

    if (res == LE_OK)
    {
        if (strlen(smscMdmStr) > (len - 1))
        {
            res = LE_OVERFLOW;
        }
        else
        {
            strncpy(telPtr, smscMdmStr, len);
        }
    }

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the SMS center address.
 *
 * SMS center address number is defined in ITU-T recommendations E.164/E.163.
 * E.164 numbers can have a maximum of fifteen digits and are usually written with a '+' prefix.
 *
 * @return
 *  - LE_FAULT         Service is not available.
 *  - LE_OK            Function succeeded.
 *
 * @note If the SMS center address number is too long (max LE_MDMDEFS_PHONE_NUM_MAX_LEN digits), it
 *       is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetSmsCenterAddress
(
    const char*            telPtr  ///< [IN] SMS center address number string.
)
{
    le_result_t res = LE_FAULT;

    if(strlen(telPtr) > (LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1))
    {
        LE_KILL_CLIENT("strlen(telPtr) > %d", (LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1));
        return res;
    }

    le_sem_Wait(SmsSem);
    res = pa_sms_SetSmsc(telPtr);
    le_sem_Post(SmsSem);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the preferred SMS storage for incoming messages.
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetPreferredStorage
(
    le_sms_Storage_t  prefStorage  ///< IN storage parameter.
)
{
    return (pa_sms_SetPreferredStorage(prefStorage));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the preferred SMS storage for incoming messages.
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetPreferredStorage
(
    le_sms_Storage_t*  prefStorage  ///< OUT storage parameter.
)
{
    return (pa_sms_GetPreferredStorage(prefStorage));
}

//--------------------------------------------------------------------------------------------------
/**
 * Activate Cell Broadcast message notification.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_ActivateCellBroadcast
(
    void
)
{
    le_result_t res;

    le_sem_Wait(SmsSem);
    res = pa_sms_ActivateCellBroadcast(PA_SMS_PROTOCOL_GSM);
    le_sem_Post(SmsSem);

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deactivate Cell Broadcast message notification.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_DeactivateCellBroadcast
(
    void
)
{
    le_result_t res;

    le_sem_Wait(SmsSem);
    res = pa_sms_DeactivateCellBroadcast(PA_SMS_PROTOCOL_GSM);
    le_sem_Post(SmsSem);

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Activate CDMA Cell Broadcast message notification.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_ActivateCdmaCellBroadcast
(
    void
)
{
    le_result_t res;

    le_sem_Wait(SmsSem);
    res = pa_sms_ActivateCellBroadcast(PA_SMS_PROTOCOL_CDMA);
    le_sem_Post(SmsSem);

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deactivate CDMA Cell Broadcast message notification.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_DeactivateCdmaCellBroadcast
(
    void
)
{
    le_result_t res;

    le_sem_Wait(SmsSem);
    res = pa_sms_DeactivateCellBroadcast(PA_SMS_PROTOCOL_CDMA);
    le_sem_Post(SmsSem);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add Cell Broadcast message Identifiers range.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_AddCellBroadcastIds
(
    uint16_t fromId,
        ///< [IN]
        ///< Starting point of the range of cell broadcast message identifier.

    uint16_t toId
        ///< [IN]
        ///< Ending point of the range of cell broadcast message identifier.
)
{
    le_result_t res;

    le_sem_Wait(SmsSem);
    res = pa_sms_AddCellBroadcastIds(fromId, toId);
    le_sem_Post(SmsSem);

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove Cell Broadcast message Identifiers range.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_RemoveCellBroadcastIds
(
    uint16_t fromId,
        ///< [IN]
        ///< Starting point of the range of cell broadcast message identifier.

    uint16_t toId
        ///< [IN]
        ///< Ending point of the range of cell broadcast message identifier.
)
{
    le_result_t res;

    le_sem_Wait(SmsSem);
    res = pa_sms_RemoveCellBroadcastIds(fromId, toId);
    le_sem_Post(SmsSem);

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Clear Cell Broadcast message Identifiers range.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_ClearCellBroadcastIds
(
    void
)
{
    le_result_t res;

    le_sem_Wait(SmsSem);
    res = pa_sms_ClearCellBroadcastIds();
    le_sem_Post(SmsSem);

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_BAD_PARAMETER Parameter is invalid.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_AddCdmaCellBroadcastServices
(
    le_sms_CdmaServiceCat_t serviceCat,
        ///< [IN]
        ///< Service category assignment. Reference to 3GPP2 C.R1001-D
        ///< v1.0 Section 9.3.1 Standard Service Category Assignments.

    le_sms_Languages_t language
        ///< [IN]
        ///< Language Indicator. Reference to
        ///< 3GPP2 C.R1001-D v1.0 Section 9.2.1 Language Indicator
        ///< Value Assignments
)
{
    le_result_t res;

    if ((serviceCat >= LE_SMS_CDMA_SVC_CAT_MAX) || (language >= LE_SMS_LANGUAGE_MAX))
    {
        return LE_BAD_PARAMETER;
    }

    le_sem_Wait(SmsSem);
    res = pa_sms_AddCdmaCellBroadcastServices(serviceCat, language);
    le_sem_Post(SmsSem);

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_BAD_PARAMETER Parameter is invalid.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_RemoveCdmaCellBroadcastServices
(
    le_sms_CdmaServiceCat_t serviceCat,
        ///< [IN]
        ///< Service category assignment. Reference to 3GPP2 C.R1001-D
        ///< v1.0 Section 9.3.1 Standard Service Category Assignments.

    le_sms_Languages_t language
        ///< [IN]
        ///< Language Indicator. Reference to
        ///< 3GPP2 C.R1001-D v1.0 Section 9.2.1 Language Indicator
        ///< Value Assignments.
)
{
    le_result_t res;

    if ((serviceCat >= LE_SMS_CDMA_SVC_CAT_MAX) || (language >= LE_SMS_LANGUAGE_MAX))
    {
        return LE_BAD_PARAMETER;
    }

    le_sem_Wait(SmsSem);
    res = pa_sms_RemoveCdmaCellBroadcastServices(serviceCat, language);
    le_sem_Post(SmsSem);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_ClearCdmaCellBroadcastServices
(
    void
)
{
    le_result_t res;

    le_sem_Wait(SmsSem);
    res = pa_sms_ClearCdmaCellBroadcastServices();
    le_sem_Post(SmsSem);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the number of messages successfully received or sent since last counter reset.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetCount
(
    le_sms_Type_t messageType,      ///< Message type.
    int32_t*      messageCountPtr   ///< Number of messages.
)
{
    if (!messageCountPtr)
    {
        LE_KILL_CLIENT("messageCountPtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    switch (messageType)
    {
        case LE_SMS_TYPE_RX:
            *messageCountPtr = MessageStats.rxCount;
            break;

        case LE_SMS_TYPE_TX:
            *messageCountPtr = MessageStats.txCount;
            break;

        case LE_SMS_TYPE_BROADCAST_RX:
            *messageCountPtr = MessageStats.rxCbCount;
            break;

        default:
            LE_ERROR("Unknown message type %d", messageType);
            *messageCountPtr = 0;
            return LE_BAD_PARAMETER;
    }

    LE_DEBUG("Type=%d, count=%d", messageType, *messageCountPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start to count the messages successfully received and sent.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_StartCount
(
    void
)
{
    LE_DEBUG("Start message counting");

    // Start to count the messages.
    SetCountingState(true);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop to count the messages successfully received and sent.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_StopCount
(
    void
)
{
    LE_DEBUG("Stop message counting");

    // Stop to count the messages.
    SetCountingState(false);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the count of messages successfully received and sent.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_ResetCount
(
    void
)
{
    LE_DEBUG("Reset message counters");

    // Reset the message count for all types.
    SetMessageCount(LE_SMS_TYPE_RX, 0);
    SetMessageCount(LE_SMS_TYPE_TX, 0);
    SetMessageCount(LE_SMS_TYPE_BROADCAST_RX, 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable SMS Status Report for outgoing messages.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_EnableStatusReport
(
    void
)
{
    SetStatusReportState(true);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable SMS Status Report for outgoing messages.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_DisableStatusReport
(
    void
)
{
    SetStatusReportState(false);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get SMS Status Report activation state.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  Parameter is invalid.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_IsStatusReportEnabled
(
    bool* enabledPtr    ///< [OUT] True when SMS Status Report is enabled, false otherwise.
)
{
    if (!enabledPtr)
    {
        LE_ERROR("NULL pointer!");
        return LE_BAD_PARAMETER;
    }

    *enabledPtr = StatusReportActivation;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get TP-Message-Reference of a message. Message type should be either a SMS Status Report or an
 * outgoing SMS.
 * TP-Message-Reference is defined in 3GPP TS 23.040 section 9.2.3.6.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_FAULT          Function failed.
 *  - LE_UNAVAILABLE    Outgoing SMS message is not sent.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTpMr
(
    le_sms_MsgRef_t msgRef, ///< [IN]  Reference to the message object.
    uint8_t* tpMrPtr        ///< [OUT] 3GPP TS 23.040 TP-Message-Reference.
)
{
    le_sms_Msg_t* msgPtr;

    if (!tpMrPtr)
    {
        LE_ERROR("NULL pointer!");
        return LE_BAD_PARAMETER;
    }

    msgPtr = le_ref_Lookup(MsgRefMap, msgRef);
    if (!msgPtr)
    {
        LE_ERROR("Invalid reference (%p) provided!", msgRef);
        return LE_BAD_PARAMETER;
    }

    if ((msgPtr->type != LE_SMS_TYPE_STATUS_REPORT) && (msgPtr->type != LE_SMS_TYPE_TX))
    {
        LE_ERROR("Cannot get message reference for this type of message (%d)", msgPtr->type);
        return LE_FAULT;
    }

    if ((msgPtr->type == LE_SMS_TYPE_TX) && (msgPtr->pdu.status != LE_SMS_SENT))
    {
        LE_ERROR("Cannot get message reference before SMS message is sent");
        return LE_UNAVAILABLE;
    }

    *tpMrPtr = msgPtr->messageReference;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get TP-Recipient-Address of SMS Status Report.
 * TP-Recipient-Address is defined in 3GPP TS 23.040 section 9.2.3.14.
 * TP-Recipient-Address Type-of-Address is defined in 3GPP TS 24.011 section 8.2.5.2.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The Recipient Address length exceeds the length of the provided buffer.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTpRa
(
    le_sms_MsgRef_t msgRef, ///< [IN]  Reference to the message object.
    uint8_t* toraPtr,       ///< [OUT] 3GPP TS 24.011 TP-Recipient-Address Type-of-Address.
    char* raPtr,            ///< [OUT] 3GPP TS 23.040 TP-Recipient-Address.
    size_t raSize           ///< [IN]  3GPP TS 23.040 TP-Recipient-Address buffer length.
)
{
    le_sms_Msg_t* msgPtr;

    if ((!toraPtr) || (!raPtr))
    {
        LE_ERROR("NULL pointer!");
        return LE_BAD_PARAMETER;
    }

    msgPtr = le_ref_Lookup(MsgRefMap, msgRef);
    if (!msgPtr)
    {
        LE_ERROR("Invalid reference (%p) provided!", msgRef);
        return LE_BAD_PARAMETER;
    }

    if (msgPtr->type != LE_SMS_TYPE_STATUS_REPORT)
    {
        LE_ERROR("It is not a SMS Status Report Message");
        return LE_FAULT;
    }

    if (strlen(msgPtr->tel) > (raSize - 1))
    {
        LE_ERROR("Output buffer is too small");
        return LE_OVERFLOW;
    }

    strncpy(raPtr, msgPtr->tel, raSize);
    *toraPtr = msgPtr->typeOfAddress;
    LE_DEBUG("Recipient Address: %s, Type of Address: %d", raPtr, *toraPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get TP-Service-Centre-Time-Stamp of SMS Status Report.
 * TP-Service-Centre-Time-Stamp is defined in 3GPP TS 23.040 section 9.2.3.11.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The SC Timestamp length exceeds the length of the provided buffer.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTpScTs
(
    le_sms_MsgRef_t msgRef, ///< [IN]  Reference to the message object.
    char* sctsPtr,          ///< [OUT] 3GPP TS 23.040 TP-Service-Centre-Time-Stamp.
    size_t sctsSize         ///< [IN]  3GPP TS 23.040 TP-Service-Centre-Time-Stamp buffer length.
)
{
    le_sms_Msg_t* msgPtr;

    if (!sctsPtr)
    {
        LE_ERROR("NULL pointer!");
        return LE_BAD_PARAMETER;
    }

    msgPtr = le_ref_Lookup(MsgRefMap, msgRef);
    if (!msgPtr)
    {
        LE_ERROR("Invalid reference (%p) provided!", msgRef);
        return LE_BAD_PARAMETER;
    }

    if (msgPtr->type != LE_SMS_TYPE_STATUS_REPORT)
    {
        LE_ERROR("It is not a SMS Status Report Message");
        return LE_FAULT;
    }

    if (strlen(msgPtr->timestamp) > (sctsSize - 1))
    {
        LE_ERROR("Output buffer is too small");
        return LE_OVERFLOW;
    }

    strncpy(sctsPtr, msgPtr->timestamp, sctsSize);
    LE_DEBUG("Service Centre Timestamp: %s", sctsPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get TP-Discharge-Time of SMS Status Report.
 * TP-Discharge-Time is defined in 3GPP TS 23.040 section 9.2.3.13.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The Discharge Time length exceeds the length of the provided buffer.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTpDt
(
    le_sms_MsgRef_t msgRef, ///< [IN]  Reference to the message object.
    char* dtPtr,            ///< [OUT] 3GPP TS 23.040 TP-Discharge-Time.
    size_t dtSize           ///< [IN]  3GPP TS 23.040 TP-Discharge-Time buffer length.
)
{
    le_sms_Msg_t* msgPtr;

    if (!dtPtr)
    {
        LE_ERROR("NULL pointer!");
        return LE_BAD_PARAMETER;
    }

    msgPtr = le_ref_Lookup(MsgRefMap, msgRef);
    if (!msgPtr)
    {
        LE_ERROR("Invalid reference (%p) provided!", msgRef);
        return LE_BAD_PARAMETER;
    }

    if (msgPtr->type != LE_SMS_TYPE_STATUS_REPORT)
    {
        LE_ERROR("It is not a SMS Status Report Message");
        return LE_FAULT;
    }

    if (strlen(msgPtr->dischargeTime) > (dtSize - 1))
    {
        LE_ERROR("Output buffer is too small");
        return LE_OVERFLOW;
    }

    strncpy(dtPtr, msgPtr->dischargeTime, dtSize);
    LE_DEBUG("Discharge Time: %s", dtPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get TP-Status of SMS Status Report.
 * TP-Status is defined in 3GPP TS 23.040 section 9.2.3.15.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTpSt
(
    le_sms_MsgRef_t msgRef, ///< [IN]  Reference to the message object.
    uint8_t* stPtr          ///< [OUT] 3GPP TS 23.040 TP-Status.
)
{
    le_sms_Msg_t* msgPtr;

    if (!stPtr)
    {
        LE_ERROR("NULL pointer!");
        return LE_BAD_PARAMETER;
    }

    msgPtr = le_ref_Lookup(MsgRefMap, msgRef);
    if (!msgPtr)
    {
        LE_ERROR("Invalid reference (%p) provided!", msgRef);
        return LE_BAD_PARAMETER;
    }

    if (msgPtr->type != LE_SMS_TYPE_STATUS_REPORT)
    {
        LE_ERROR("It is not a SMS Status Report Message");
        return LE_FAULT;
    }

    *stPtr = msgPtr->status;
    LE_DEBUG("Status: %d", *stPtr);

    return LE_OK;
}
