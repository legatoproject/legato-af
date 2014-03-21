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
 * In case of listing the received messages (see le_sms_msg_CreateRxMsgList()), the message objects
 * are queued to the 'StoredRxMsgList' as well.
 *
 * The sending case:
 * The message object must be created by the client. The client can populate the message with the
 * 'setters functions' like le_sms_msg_SetDestination() and le_sms_msg_SetText().
 * Then, the client must call le_sms_msg_Send() to actually send the message. le_sms_msg_Send()
 * first verifies the consistency of the main elements of the object like the telephone number or
 * the message length, then it encodes the message in PDU if it is a Text or a Binary message, and
 * finally it forwards the message to the modem for sending.
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2012. All rights reserved. Use of this work is subject to license.
 *
 */
#include "legato.h"
#include "le_sms.h"
#include "pa_sms.h"
#include "smspdu.h"



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
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_OF_SMS_MSG    MAX_NUM_OF_SMS_MSG_IN_STORAGE

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Message List objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_OF_LIST    128

//--------------------------------------------------------------------------------------------------
/**
 * Message Type.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_SMS_TYPE_RECEIVED    = 0, ///< Received message.
    LE_SMS_TYPE_SUBMITTED   = 1, ///< Message submitted to transmission.
    LE_SMS_TYPE_PDU         = 2, ///< PDU message.
}
le_sms_msg_Type_t;

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
    bool                readonly;                            ///< Flag for Read-Only message.
    bool                inAList;                             ///< The message belongs to a list or not
    le_sms_msg_Format_t format;                              ///< SMS Message Format.
    le_sms_msg_Type_t   type;                                ///< SMS Message Type.
    uint32_t            storageIdx;                          ///< SMS Message index in storage.
    char                tel[LE_SMS_TEL_NMBR_MAX_LEN];        ///< Telephone number of the message
                                                             ///  (in text mode), or NULL (in PDU mode).
    char                timestamp[LE_SMS_TIMESTAMP_MAX_LEN]; ///< SMS time stamp (in text mode).
    pa_sms_Pdu_t        pdu;                                 ///< SMS PDU
    bool                pduReady;                            ///< Whether the PDU value is ready or not
    union {
        char            text[LE_SMS_TEXT_MAX_LEN];           ///< SMS text
        uint8_t         binary[LE_SMS_BINARY_MAX_LEN];       ///< SMS binary
    };
    size_t              userdataLen;                         ///< Length of data associated with SMS
                                                             ///  formats text or binary
}le_sms_Msg_t;


//--------------------------------------------------------------------------------------------------
/**
 * Data structure to keep a list of the references created with 'CreateRxMsgList' function.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sms_MsgReference
{
    le_sms_msg_Ref_t    msgRef;     ///< The message reference.
    le_dls_Link_t       listLink;   ///< Object node link (for msg listing).
}le_sms_MsgReference_t;


//--------------------------------------------------------------------------------------------------
/**
 * List message structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sms_msg_List
{
    le_dls_List_t   list;
    le_dls_Link_t*  currentLink;
}le_sms_msg_List_t;


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
 * Event ID for New SMS message notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewSmsEventId;



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
            // Get the node from MsgList
            nodePtr = CONTAINER_OF(linkPtr, le_sms_MsgReference_t, listLink);

            le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, nodePtr->msgRef);
            if (msgPtr == NULL)
            {
                LE_CRIT("Invalid reference (%p) provided!", nodePtr);
                return;
            }

            // Invalidate the Safe Reference.
            le_ref_DeleteRef(MsgRefMap, nodePtr->msgRef);

            // release the message object
            le_mem_Release(msgPtr);

            // Move to the next node.
            linkPtr = le_dls_Pop(msgListPtr);
        } while (linkPtr != NULL);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Create and Populate a new message object from a PDU.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_sms_Msg_t* CreateAndPopulateMessage
(
    uint32_t            storageIdx,
    pa_sms_Pdu_t*       messagePduPtr,
    pa_sms_Message_t*   messageConvertedPtr
)
{
    // Create and populate the SMS message object (it is Read Only).
    le_sms_Msg_t  *newSmsMsgObjPtr;

    // Create the message node.
    newSmsMsgObjPtr = (le_sms_Msg_t*)le_mem_ForceAlloc(MsgPool);
    newSmsMsgObjPtr->timestamp[0] = '\0';
    newSmsMsgObjPtr->tel[0] = '\0';
    newSmsMsgObjPtr->text[0] = '\0';
    newSmsMsgObjPtr->userdataLen = 0;
    newSmsMsgObjPtr->pduReady = false;
    newSmsMsgObjPtr->pdu.status = LE_SMS_UNSENT;
    newSmsMsgObjPtr->pdu.dataLen = 0;
    newSmsMsgObjPtr->inAList = false;
    newSmsMsgObjPtr->readonly = true;
    newSmsMsgObjPtr->storageIdx = storageIdx;
    switch (messageConvertedPtr->type)
    {
        case PA_SMS_SMS_DELIVER:
            newSmsMsgObjPtr->type = LE_SMS_TYPE_RECEIVED;
            newSmsMsgObjPtr->format = messageConvertedPtr->smsDeliver.format;
            break;
        case PA_SMS_PDU:
            newSmsMsgObjPtr->type = LE_SMS_TYPE_PDU;
            newSmsMsgObjPtr->format = LE_SMS_FORMAT_PDU;
            break;
        default:
            // CreateAndPopulateMessage is called by my internal handler. I don't know how to face
            // this error, so it is a fatal error.
            LE_FATAL("Unknown or not supported SMS format %d", newSmsMsgObjPtr->format);
            break;
    }

    /* Copy PDU */
    memcpy(&(newSmsMsgObjPtr->pdu), messagePduPtr, sizeof(pa_sms_Pdu_t));

    switch (newSmsMsgObjPtr->format)
    {
        case LE_SMS_FORMAT_PDU:
            // CreateAndPopulateMessage is called by my internal handler. I don't know how to face
            // this error, so it is a fatal error.
            LE_ASSERT(messagePduPtr->dataLen <= LE_SMS_PDU_MAX_LEN);
            break;
        case LE_SMS_FORMAT_BINARY:
            // CreateAndPopulateMessage is called by my internal handler. I don't know how to face
            // this error, so it is a fatal error.
            LE_ASSERT(messageConvertedPtr->smsDeliver.dataLen <= LE_SMS_BINARY_MAX_LEN);
            newSmsMsgObjPtr->userdataLen = messageConvertedPtr->smsDeliver.dataLen;
            memcpy(newSmsMsgObjPtr->binary,
                    messageConvertedPtr->smsDeliver.data,
                    LE_SMS_BINARY_MAX_LEN);
            break;
        case LE_SMS_FORMAT_TEXT:
            // CreateAndPopulateMessage is called by my internal handler. I don't know how to face
            // this error, so it is a fatal error.
            LE_ASSERT(messageConvertedPtr->smsDeliver.dataLen < LE_SMS_TEXT_MAX_LEN);
            newSmsMsgObjPtr->userdataLen = messageConvertedPtr->smsDeliver.dataLen;
            memcpy(newSmsMsgObjPtr->text,
                    messageConvertedPtr->smsDeliver.data,
                    LE_SMS_TEXT_MAX_LEN);
            break;
        default:
            // CreateAndPopulateMessage is called by my internal handler. I don't know how to face
            // this error, so it is a fatal error.
            LE_FATAL("Unknown SMS format %d", newSmsMsgObjPtr->format);
            break;
    }

    if (newSmsMsgObjPtr->format != LE_SMS_FORMAT_PDU)
    {
        memcpy(newSmsMsgObjPtr->tel, messageConvertedPtr->smsDeliver.oa, LE_SMS_TEL_NMBR_MAX_LEN);
        memcpy(newSmsMsgObjPtr->timestamp,
               messageConvertedPtr->smsDeliver.scts,
               LE_SMS_TIMESTAMP_MAX_LEN);
    }

    return (newSmsMsgObjPtr);
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
    le_sms_msg_List_t*  msgListObjPtr, ///< List of received messages.
    uint32_t            numOfMsg,      ///< Number of message to read from memory
    uint32_t*           arrayPtr       ///< Array of message indexes
)
{
    le_result_t  pa_res=LE_OK;
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

    // Get Unread messages
    for (i=0 ; i < numOfMsg ; i++)
    {
        if ((pa_res=pa_sms_RdPDUMsgFromMem(arrayPtr[i], &messagePdu)) != LE_OK)
        {
            LE_ERROR("pa_sms_RdMsgFromMem failed");
            return LE_FAULT;
        }

        // Try to decode message
        pa_sms_Message_t messageConverted;

        if (smspdu_Decode(messagePdu.data, &messageConverted) == LE_OK)
        {
            if (messagePdu.dataLen <= LE_SMS_PDU_MAX_LEN)
            {
                if (messageConverted.type != PA_SMS_SMS_SUBMIT)
                {
                    le_sms_MsgReference_t* newReferencePtr = (le_sms_MsgReference_t*)le_mem_ForceAlloc(ReferencePool);

                    le_sms_Msg_t* newSmsMsgObjPtr = CreateAndPopulateMessage(arrayPtr[i],
                                                                             &messagePdu,
                                                                             &messageConverted);
                    newSmsMsgObjPtr->inAList = true;
                    // Create a Safe Reference for this Message object.
                    newReferencePtr->msgRef = le_ref_CreateRef(MsgRefMap, newSmsMsgObjPtr);
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
                LE_ERROR("PDU length out of range (%u) for message %d !",
                         messagePdu.dataLen,
                         arrayPtr[i]);
            }
        }
        else
        {
            LE_WARN("Could not decode the message (idx.%d)", arrayPtr[i]);
        }
    }

    return numOfQueuedMsg;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to list the Received Messages present in the message storage.
 *
 * @return LE_NOT_POSSIBLE   The current modem state does not allow to list messages.
 * @return LE_NO_MEMORY      The message storage is not available.
 * @return LE_FAULT          The function failed to list messages.
 * @return A positive value  The number of messages present in the storage area, it can be equal to
 *                           zero (no messages).
 */
//--------------------------------------------------------------------------------------------------
static int32_t ListReceivedMessages
(
    le_sms_msg_List_t* msgListObjPtr ///< List of received messages.
)
{
    le_result_t  result = LE_OK;
    uint32_t     numTot, numOfUnread, numOfRead;
    uint32_t     msgCount = 0;
    /* Arrays to store IDs of messages saved in the storage area. There are one array for READ
     * message and another one for UNREAD message.*/
    uint32_t     idxArrayRead[MAX_NUM_OF_SMS_MSG_IN_STORAGE]={0};
    uint32_t     idxArrayUnread[MAX_NUM_OF_SMS_MSG_IN_STORAGE]={0};

    if (msgListObjPtr == NULL)
    {
        LE_FATAL("msgListObjPtr is NULL !");
    }

    /* Get Indexes */
    result = pa_sms_ListMsgFromMem(LE_SMS_RX_UNREAD, &numOfUnread, idxArrayUnread);
    if (result != LE_OK)
    {
        LE_ERROR("pa_sms_ListMsgFromMem failed");
        return result;
    }

    result = pa_sms_ListMsgFromMem(LE_SMS_RX_READ, &numOfRead, idxArrayRead);
    if (result != LE_OK)
    {
        LE_ERROR("pa_sms_ListMsgFromMem failed");
        return result;
    }

    numTot = numOfUnread + numOfRead;

    /* Retrieve messages */
    if ((numTot > 0) && (numTot < MAX_NUM_OF_SMS_MSG_IN_STORAGE))
    {
        int32_t retValue;

        retValue = GetMessagesFromMem(msgListObjPtr, numOfUnread, idxArrayUnread);
        if(retValue == LE_FAULT)
        {
            return LE_FAULT;
        }
        msgCount = retValue;

        retValue = GetMessagesFromMem(msgListObjPtr, numOfRead, idxArrayRead);
        if(retValue == LE_FAULT)
        {
            return LE_FAULT;
        }
        msgCount += retValue;
    }
    else if (numTot == 0)
    {
        result = LE_OK;
        msgCount = 0;
    }
    else
    {
        return LE_FAULT;
    }

    return msgCount;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to prepare a message to be send by converting it's content to PDU data.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EncodeMessageToPdu
(
    le_sms_Msg_t* msgPtr         ///< [IN] The message to encode
)
{
    le_result_t result = LE_FAULT;

    if (msgPtr == NULL)
    {
        LE_FATAL("msgPtr is NULL !");
    }

    switch (msgPtr->format)
    {
        case LE_SMS_FORMAT_TEXT:
            LE_DEBUG("Try to encode Text Msg %p, tel.%s, text.%s, userdataLen %zd",
                    msgPtr, msgPtr->tel, msgPtr->text, msgPtr->userdataLen);

            /* @todo send split messages */
            result = smspdu_Encode((const uint8_t*)msgPtr->text,
                                   msgPtr->userdataLen,
                                   msgPtr->tel,
                                   SMSPDU_GSM_7_BITS,
                                   &(msgPtr->pdu));

            break;

        case LE_SMS_FORMAT_BINARY:
            LE_DEBUG("Try to encode Binary Msg.%p, tel.%s, binary.%p, userdataLen.%zd",
                    msgPtr, msgPtr->tel, msgPtr->binary, msgPtr->userdataLen);

            /* @todo send split messages */
            result = smspdu_Encode(msgPtr->binary,
                                   msgPtr->userdataLen,
                                   msgPtr->tel,
                                   SMSPDU_8_BITS,
                                   &(msgPtr->pdu));

            break;

        case LE_SMS_FORMAT_PDU:
            result = LE_OK; /* Conversion from PDU to PDU succeeded ... */
            break;

        case LE_SMS_FORMAT_UNKNOWN:
            result = LE_FAULT; /* Unknown format */
            break;
    }

    if (result != LE_OK)
    {
        LE_DEBUG("Failed to encode the message");
    }
    else
    {
        msgPtr->pduReady = true;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer New SMS message Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerNewSmsHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_sms_msg_Ref_t*              referencePtr = reportPtr;
    le_sms_msg_RxMessageHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*referencePtr, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * New SMS message handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NewSmsHandler
(
    uint32_t* msgIdPtr
)
{
    pa_sms_Pdu_t   messagePdu;

    LE_DEBUG("Handler Function called with message ID %d", *msgIdPtr);

    if (pa_sms_RdPDUMsgFromMem(*msgIdPtr, &messagePdu) != LE_OK)
    {
        LE_ERROR("pa_sms_RdPDUMsgFromMem failed");
    }
    else
    {
        // Try to decode message
        pa_sms_Message_t    messageConverted;

        if ( smspdu_Decode(messagePdu.data, &messageConverted) == LE_OK )
        {
            if (messagePdu.dataLen <= LE_SMS_PDU_MAX_LEN)
            {
                if (messageConverted.type == PA_SMS_SMS_DELIVER)
                {
                    le_sms_Msg_t* newSmsMsgObjPtr = CreateAndPopulateMessage(*msgIdPtr,
                                                                             &messagePdu,
                                                                             &messageConverted);

                    le_sms_msg_Ref_t newSmsMsgRef = le_ref_CreateRef(MsgRefMap, newSmsMsgObjPtr);
                    // Notify all the registered client's handlers
                    le_event_Report(NewSmsEventId, (void*)&newSmsMsgRef, sizeof(le_sms_msg_Ref_t));
                    LE_DEBUG("All the registered client's handlers notified with %p", newSmsMsgRef);
                }
                else
                {
                    LE_DEBUG("this messagePdu type %d is not supported yet", messageConverted.type);
                }
            }
            else
            {
                LE_ERROR("PDU length out of range (%u) !", messagePdu.dataLen);
            }
        }
        else
        {
            LE_DEBUG("Could not decode the message");
        }
    }
    le_mem_Release(msgIdPtr);
}


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the SMS operations component
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_msg_Init
(
    void
)
{
    // Create a pool for Message objects
    MsgPool = le_mem_CreatePool("SmsMsgPool", sizeof(le_sms_Msg_t));
    le_mem_ExpandPool(MsgPool, MAX_NUM_OF_SMS_MSG);

    // Create the Safe Reference Map to use for Message object Safe References.
    MsgRefMap = le_ref_CreateMap("SmsMsgMap", MAX_NUM_OF_SMS_MSG);

    // Create a pool for List objects
    ListPool = le_mem_CreatePool("ListSmsPool", sizeof(le_sms_msg_List_t));
    le_mem_ExpandPool(ListPool, MAX_NUM_OF_LIST);

    // Create the Safe Reference Map to use for List object Safe References.
    ListRefMap = le_ref_CreateMap("ListSmsMap", MAX_NUM_OF_LIST);

    // Create a pool for Message references list
    ReferencePool = le_mem_CreatePool("SmsReferencePool", sizeof(le_sms_MsgReference_t));

    // Create an event Id for new incoming SMS messages
    NewSmsEventId = le_event_CreateId("NewSms", sizeof(le_sms_msg_Ref_t));

    // Register a handler function for new message indication
    LE_FATAL_IF((pa_sms_SetNewMsgHandler(NewSmsHandler) != LE_OK),
                "Add pa_sms_SetNewMsgHandler failed");
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
le_sms_msg_Ref_t le_sms_msg_Create
(
    void
)
{
    le_sms_Msg_t  *msgPtr;

    // Create the message node.
    msgPtr = (le_sms_Msg_t*)le_mem_ForceAlloc(MsgPool);
    msgPtr->timestamp[0] = '\0';
    msgPtr->tel[0] = '\0';
    msgPtr->text[0] = '\0';
    msgPtr->userdataLen = 0;
    msgPtr->pduReady = false;
    msgPtr->pdu.status = LE_SMS_UNSENT;
    msgPtr->pdu.dataLen = 0;
    msgPtr->readonly = false;
    msgPtr->inAList = false;

    // Create and return a Safe Reference for this Message object.
    return le_ref_CreateRef(MsgRefMap, msgPtr);
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
void le_sms_msg_Delete
(
    le_sms_msg_Ref_t  msgRef    ///< [IN] The pointer to the message data structure.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return;
    }

    if (msgPtr->inAList)
    {
        LE_KILL_CLIENT("This message (ref.%p) belongs to a Rx List ! Call 'DeleteList' instead.", msgRef);
        return;
    }
    // Invalidate the Safe Reference.
    le_ref_DeleteRef(MsgRefMap, msgRef);

    // release the message object
    le_mem_Release(msgPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the message format (text or PDU).
 *
 * @return The message format.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_msg_Format_t le_sms_msg_GetFormat
(
    le_sms_msg_Ref_t    msgRef   ///< [IN] The pointer to the message data structure.
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
 * This function must be called to set the Telephone destination number.
 *
 * The Telephone number is defined in ITU-T recommendations E.164/E.163.
 * E.164 numbers can have a maximum of fifteen digits and are usually written with a ‘+’ prefix.
 *
 * @return LE_NOT_PERMITTED The message is Read-Only.
 * @return LE_BAD_PARAMETER The Telephone destination number length is equal to zero.
 * @return LE_OVERFLOW      The Telephone destination number is too long.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_SetDestination
(
    le_sms_msg_Ref_t  msgRef,  ///< [IN] The pointer to the message data structure.
    const char*       destPtr  ///< [IN] The telephone number string.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }
    if (destPtr == NULL)
    {
        LE_KILL_CLIENT("destPtr is NULL !");
        return LE_FAULT;
    }

    if(msgPtr->readonly)
    {
        return LE_NOT_PERMITTED;
    }

    if(!strlen(destPtr))
    {
        return LE_BAD_PARAMETER;
    }

    msgPtr->pduReady = false; // PDU must be regenerated
    return (le_utf8_Copy(msgPtr->tel, destPtr, sizeof(msgPtr->tel), NULL));
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
le_result_t le_sms_msg_GetSenderTel
(
    le_sms_msg_Ref_t msgRef,  ///< [IN] The pointer to the message data structure.
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

    return (le_utf8_Copy(telPtr, msgPtr->tel, len, NULL));
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
le_result_t le_sms_msg_GetTimeStamp
(
    le_sms_msg_Ref_t msgRef,       ///< [IN] The pointer to the message data structure.
    char*            timestampPtr, ///< [OUT] The message time stamp (in text mode).
                                   ///        string format: "yy/MM/dd,hh:mm:ss+/-zz"
                                   ///        (Year/Month/Day,Hour:Min:Seconds+/-TimeZone)
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

    // Only received messages are read only
    if(!msgPtr->readonly)
    {
        LE_ERROR("Error.%d : It is not a received message", LE_NOT_PERMITTED);
        return LE_NOT_PERMITTED;
    }

    return (le_utf8_Copy(timestampPtr, msgPtr->timestamp, len, NULL));
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
size_t le_sms_msg_GetUserdataLen
(
    le_sms_msg_Ref_t msgRef ///< [IN] The pointer to the message data structure.
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
size_t le_sms_msg_GetPDULen
(
    le_sms_msg_Ref_t msgRef ///< [IN] The pointer to the message data structure.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return 0;
    }

    if (!msgPtr->pduReady)
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
 * This function must be called to set the Text Message content.
 *
 * @return LE_NOT_PERMITTED The message is Read-Only.
 * @return LE_BAD_PARAMETER The text message length is equal to zero.
 * @return LE_OUT_OF_RANGE  The message is too long.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_SetText
(
    le_sms_msg_Ref_t      msgRef, ///< [IN] The pointer to the message data structure.
    const char*           textPtr ///< [IN] The SMS text.
)
{
    size_t        length;
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

    if(msgPtr->readonly)
    {
        return LE_NOT_PERMITTED;
    }

    if(!strlen(textPtr))
    {
        return LE_BAD_PARAMETER;
    }

    length = strnlen(textPtr, LE_SMS_TEXT_MAX_LEN+1);
    if (!length)
    {
        return LE_BAD_PARAMETER;
    }

    if (length > LE_SMS_TEXT_MAX_LEN)
    {
        return LE_OUT_OF_RANGE;
    }

    msgPtr->format = LE_SMS_FORMAT_TEXT;
    msgPtr->userdataLen=length;
    msgPtr->pduReady = false;
    LE_DEBUG("try to copy data %s, len.%zd @ msgPtr->text.%p for msgPtr.%p",
             textPtr, length, msgPtr->text, msgPtr);

    return (le_utf8_Copy(msgPtr->text, textPtr, sizeof(msgPtr->text), NULL));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the binary message content.
 *
 * @return LE_NOT_PERMITTED The message is Read-Only.
 * @return LE_BAD_PARAMETER The length of the data is equal to zero.
 * @return LE_OUT_OF_RANGE  The message is too long.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_SetBinary
(
    le_sms_msg_Ref_t  msgRef, ///< [IN] The pointer to the message data structure.
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

    if(len > LE_SMS_BINARY_MAX_LEN)
    {
        return LE_OUT_OF_RANGE;
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
 * @return LE_OUT_OF_RANGE  The message is too long.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_SetPDU
(
    le_sms_msg_Ref_t  msgRef, ///< [IN] The pointer to the message data structure.
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

    if(len > LE_SMS_PDU_MAX_LEN)
    {
        return LE_OUT_OF_RANGE;
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
 * This function must be called to get the text Message.
 *
 * The output parameter is updated with the text string. If the text string exceed the
 * value of 'len' parameter, a LE_OVERFLOW error code is returned and 'text' is filled until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'text'.
 *
 * @return LE_OVERFLOW      The message length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_GetText
(
    le_sms_msg_Ref_t msgRef,  ///< [IN]  The pointer to the message data structure.
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

    return (le_utf8_Copy(textPtr, msgPtr->text, len, NULL));
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the binary Message.
 *
 * The output parameters are updated with the binary message content and the length of the raw
 * binary message in bytes. If the binary data exceed the value of 'len' input parameter, a
 * LE_OVERFLOW error code is returned and 'raw' is filled until 'len' bytes.
 *
 * @return LE_FORMAT_ERROR  Message is not in binary format
 * @return LE_OVERFLOW      The message length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_GetBinary
(
    le_sms_msg_Ref_t msgRef, ///< [IN]  The pointer to the message data structure.
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
 * @return LE_FORMAT_ERROR  Unable to encode the message in PDU.
 * @return LE_OVERFLOW      The message length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_GetPDU
(
    le_sms_msg_Ref_t msgRef, ///< [IN]  The pointer to the message data structure.
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

    if (!msgPtr->pduReady)
    {
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
le_sms_msg_RxMessageHandlerRef_t le_sms_msg_AddRxMessageHandler
(
    le_sms_msg_RxMessageHandlerFunc_t handlerFuncPtr, ///< [IN] The handler function for message
                                                       ///  reception.
    void*                              contextPtr      ///< [IN] The handler's context.
)
{
    le_event_HandlerRef_t handlerRef;

    if(handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("handlerFuncPtr is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("NewMsgHandler",
                                            NewSmsEventId,
                                            FirstLayerNewSmsHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_sms_msg_RxMessageHandlerRef_t)(handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister a handler function
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_msg_RemoveRxMessageHandler
(
    le_sms_msg_RxMessageHandlerRef_t   handlerRef ///< [IN] The handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);

    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send an SMS message.
 *
 * It verifies first if the parameters are valid, then it checks that the modem state can support
 * message sending.
 *
 * @return LE_NOT_POSSIBLE  The current modem state does not support message sending.
 * @return LE_FORMAT_ERROR  The message content is invalid.
 * @return LE_FAULT         The function failed to send the message.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_Send
(
    le_sms_msg_Ref_t    msgRef         ///< [IN] The message(s) to send.
)
{
    le_result_t   result = LE_OK;
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }

    /* Validate */
    switch (msgPtr->format)
    {
        case LE_SMS_FORMAT_TEXT:
            if ( (msgPtr->userdataLen == 0) || (msgPtr->text[0] == '\0') )
            {
                LE_ERROR("Error.%d : Text content is invalid for Message Object %p",
                        LE_FORMAT_ERROR, msgPtr);
                return LE_FORMAT_ERROR;
            }
            break;

        case LE_SMS_FORMAT_BINARY:
            if (msgPtr->userdataLen == 0)
            {
                LE_ERROR("Binary content is empty for Message Object %p", msgPtr);
                return LE_FORMAT_ERROR;
            }
            break;

        case LE_SMS_FORMAT_PDU:
            if (msgPtr->pdu.dataLen == 0)
            {
                LE_ERROR("Error.%d : No PDU content for Message Object %p", LE_FORMAT_ERROR, msgPtr);
                return LE_FORMAT_ERROR;
            }
            break;

        default:
            LE_ERROR("Error.%d : Format for Message Object %p is incorrect (%d)",
                    LE_FORMAT_ERROR, msgPtr, msgPtr->format);
            return LE_FORMAT_ERROR;
    }

    if ( (msgPtr->format != LE_SMS_FORMAT_PDU) && (msgPtr->tel[0] == '\0') )
    {
        LE_ERROR("Error.%d : Telephone number is invalid for Message Object %p", LE_FORMAT_ERROR, msgPtr);
        return LE_FORMAT_ERROR;
    }

    /* Encode */
    if (!msgPtr->pduReady)
    {
        result = EncodeMessageToPdu(msgPtr);
    }

    /* Send */
    if (result == LE_OK)
    {
        LE_DEBUG("Try to send PDU Msg %p, pdu.%p, pduLen.%u",
                  msgPtr, msgPtr->pdu.data, msgPtr->pdu.dataLen);

        result = pa_sms_SendPduMsg(msgPtr->pdu.dataLen, msgPtr->pdu.data);
    }

    if (result < 0)
    {
        LE_ERROR("Error.%d : Failed to send Message Object %p", result, msgPtr);
        result = LE_FAULT;
    }
    else
    {
        /* @todo Get message reference for acknowledge message feature */
        msgPtr->pdu.status = LE_SMS_SENT;
        result = LE_OK;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete an SMS message from the storage area.
 *
 * It verifies first if the parameter is valid, then it checks that the modem state can support
 * message deleting.
 *
 * @return LE_NOT_POSSIBLE  The current modem state does not support message deleting.
 * @return LE_FAULT         The function failed to perform the deletion.
 * @return LE_NO_MEMORY     The message storage is not available.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_DeleteFromStorage
(
    le_sms_msg_Ref_t msgRef   ///< [IN] The message to delete.
)
{
    le_result_t   resp = LE_OK;
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return LE_NOT_FOUND;
    }

    resp = pa_sms_DelMsgFromMem(msgPtr->storageIdx);

    if ((resp == LE_COMM_ERROR) || (resp == LE_TIMEOUT))
    {
        return LE_NO_MEMORY;
    }
    else
    {
        if (resp != LE_OK)
        {
            resp = LE_FAULT;
        }
        return resp;
    }
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
le_sms_msg_ListRef_t le_sms_msg_CreateRxMsgList
(
    void
)
{
    le_sms_msg_List_t* StoredRxMsgListObjPtr = (le_sms_msg_List_t*)le_mem_ForceAlloc(ListPool);

    if (StoredRxMsgListObjPtr != NULL)
    {
        StoredRxMsgListObjPtr->list = LE_DLS_LIST_INIT;
        if (ListReceivedMessages(StoredRxMsgListObjPtr) > 0)
        {
            StoredRxMsgListObjPtr->currentLink = NULL;
            // Create and return a Safe Reference for this List object.
            return le_ref_CreateRef(ListRefMap, StoredRxMsgListObjPtr);
        }
        else
        {
            return NULL;
        }
    }
    else
    {
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
void le_sms_msg_DeleteList
(
    le_sms_msg_ListRef_t     msgListRef   ///< [IN] The list of the Messages.
)
{
    le_sms_msg_List_t* listPtr = le_ref_Lookup(ListRefMap, msgListRef);

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
 * retrieved with le_sms_msg_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return le_sms_msg_Ref_t  The Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_msg_Ref_t le_sms_msg_GetFirst
(
    le_sms_msg_ListRef_t        msgListRef ///< [IN] The list of the messages.
)
{
    le_sms_MsgReference_t*  nodePtr;
    le_dls_Link_t*          msgLinkPtr;
    le_sms_msg_List_t*      listPtr = le_ref_Lookup(ListRefMap, msgListRef);

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
 * retrieved with le_sms_msg_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return le_sms_msg_Ref_t  The Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_msg_Ref_t le_sms_msg_GetNext
(
    le_sms_msg_ListRef_t        msgListRef ///< [IN] The list of the messages.
)
{
    le_sms_MsgReference_t*  nodePtr;
    le_dls_Link_t*          msgLinkPtr;
    le_sms_msg_List_t*      listPtr = le_ref_Lookup(ListRefMap, msgListRef);

    if (listPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgListRef);
        return NULL;
    }

    // Move to the next node.
    msgLinkPtr = le_dls_PeekNext(&(listPtr->list), listPtr->currentLink);
    if (msgLinkPtr != NULL)
    {
        // Get the node from MsgList
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
le_sms_msg_Status_t le_sms_msg_GetStatus
(
    le_sms_msg_Ref_t      msgRef        ///< [IN] The message.
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
void le_sms_msg_MarkRead
(
    le_sms_msg_Ref_t    msgRef         ///< [IN] The message.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return;
    }
    if (pa_sms_ChangeMessageStatus(msgPtr->storageIdx, LE_SMS_RX_READ) == LE_OK)
    {
        msgPtr->pdu.status = LE_SMS_RX_READ;
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to mark a message as 'unread'.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_msg_MarkUnread
(
    le_sms_msg_Ref_t    msgRef         ///< [IN] The message.
)
{
    le_sms_Msg_t* msgPtr = le_ref_Lookup(MsgRefMap, msgRef);

    if (msgPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", msgRef);
        return;
    }
    if (pa_sms_ChangeMessageStatus(msgPtr->storageIdx, LE_SMS_RX_UNREAD) == LE_OK)
    {
        msgPtr->pdu.status = LE_SMS_RX_UNREAD;
    }
}

