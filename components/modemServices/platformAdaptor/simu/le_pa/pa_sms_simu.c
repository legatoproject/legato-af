/**
 * @file pa_sms_simu.c
 *
 * Simulation implementation of @ref c_pa_sms API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "pa_sim.h"
#include "pa_sms.h"
#include "pa_mrc.h"

#include "pa_simu.h"
#include "pa_sms_simu.h"
#include "smsPdu.h"

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#define PA_SMS_SIMU_MAX_CONN        1
#define PA_SMS_SIMU_MAX_MSG_IN_MEM  16

static le_event_Id_t          EventNewSmsId;
static le_event_HandlerRef_t  NewSMSHandlerRef;

static int SmsServerListenFd;
static le_fdMonitor_Ref_t SmsServerMonitorRef;

typedef struct {
    bool used;
    int fd;
    le_fdMonitor_Ref_t fdMonitorRef;
}
SmsServerConnection_t;

static SmsServerConnection_t SmsServerConnections[PA_SMS_SIMU_MAX_CONN];

/** Memory **/

typedef struct {
    pa_sms_Pdu_t pduContent;
}
SmsMsgInMemory;

typedef struct {
    pa_sms_Storage_t storage;
    uint32_t         index;
}
SmsMsgRef;

#define PA_SMS_SIMU_STORAGE_CNT     PA_SMS_STORAGE_SIM

static SmsMsgInMemory SmsMem[PA_SMS_SIMU_STORAGE_CNT][PA_SMS_SIMU_MAX_MSG_IN_MEM];
static le_mem_PoolRef_t SmsMemPoolRef;

static char SmsSmsc[LE_MDMDEFS_PHONE_NUM_MAX_LEN] = PA_SIMU_SMS_DEFAULT_SMSC;

static le_result_t SmsServerHandleLocalMessage
(
    pa_sms_SimuPdu_t * sourceMsgPtr
);

static le_result_t SmsServerHandleRemoteMessage
(
    pa_sms_SimuPdu_t * sourceMsgPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Get message data in memory.
 */
//--------------------------------------------------------------------------------------------------
static SmsMsgInMemory * GetSmsMsg
(
    pa_sms_Storage_t    storage,    ///< [IN] SMS Storage used
    uint32_t            index       ///< [IN] The place of storage in memory.
)
{
    if ( (storage == PA_SMS_STORAGE_UNKNOWN) || (storage > PA_SMS_SIMU_STORAGE_CNT) )
    {
        return NULL;
    }

    if ( (index >= PA_SMS_SIMU_MAX_MSG_IN_MEM) )
    {
        return NULL;
    }

    return &SmsMem[storage-1][index];
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the message bank where incoming message should be stored.
 */
//--------------------------------------------------------------------------------------------------
static pa_sms_Storage_t GetCurrentIncomingStorage
(
    void
)
{
    le_result_t res;
    le_mrc_Rat_t rat;

    res = pa_mrc_GetRadioAccessTechInUse(&rat);
    if (LE_OK != res)
    {
        return PA_SMS_STORAGE_SIM;
    }

    if (LE_MRC_RAT_CDMA == rat)
    {
        return PA_SMS_STORAGE_NV;
    }

    return PA_SMS_STORAGE_SIM;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for a new message reception handling.
 *
 * @return LE_BAD_PARAMETER The parameter is invalid.
 * @return LE_NOT_POSSIBLE  The function failed to register a new handler.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetNewMsgHandler
(
    pa_sms_NewMsgHdlrFunc_t msgHandler   ///< [IN] The handler function to handle a new message
                                         ///       reception.
)
{

    NewSMSHandlerRef = le_event_AddHandler("NewSMSHandler",
                                         EventNewSmsId,
                                         (le_event_HandlerFunc_t) msgHandler);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for a new message reception handling.
 *
 * @return LE_NOT_POSSIBLE  The function failed to unregister the handler.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ClearNewMsgHandler
(
    void
)
{
    le_event_RemoveHandler(NewSMSHandlerRef);
    NewSMSHandlerRef = NULL;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sends a message in PDU mode.
 *
 * @return LE_NOT_POSSIBLE    The function failed to send a message in PDU mode.
 * @return LE_TIMEOUT         No response was received from the Modem.
 * @return a positive value   The function succeeded. The value represents the message reference.
 */
//--------------------------------------------------------------------------------------------------
int32_t pa_sms_SendPduMsg
(
    pa_sms_Protocol_t        protocol,   ///< [IN] protocol to use
    uint32_t                 length,     ///< [IN] The length of the TP data unit in bytes.
    const uint8_t           *dataPtr,    ///< [IN] The message.
    pa_sms_SendingErrCode_t *errorCode   ///< [OUT] The error code.
)
{
    le_result_t res;
    union {
        pa_sms_SimuPdu_t header;
        char buffer[1024];
    } txBuffer;

    if (!mrc_simu_IsOnline())
    {
        LE_WARN("Not sending message because we're offline.");
        return LE_NOT_POSSIBLE;
    }

    LE_INFO("Sending PDU message (length=%u protocol=%u)", length, protocol);

    txBuffer.header.protocol = protocol;

    res = pa_sim_GetSubscriberPhoneNumber( (char *)txBuffer.header.origAddress, LE_MDMDEFS_PHONE_NUM_MAX_LEN);
    LE_FATAL_IF(res != LE_OK, "Unable to get subscriber phone number.");

    txBuffer.header.destAddress[0] = '\0';

    if ( length >= (sizeof(txBuffer) - offsetof(pa_sms_SimuPdu_t, data)) )
    {
        LE_WARN("PDU message is too big");
        return LE_OUT_OF_RANGE;
    }

    txBuffer.header.dataLen = length;
    memcpy(txBuffer.header.data, dataPtr, length);

    SmsServerHandleLocalMessage( &(txBuffer.header) );

    return 1;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the message from the preferred message storage.
 *
 * @return LE_NOT_POSSIBLE The function failed to get the message from the preferred message
 *                         storage.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_RdPDUMsgFromMem
(
    uint32_t            index,      ///< [IN]  The place of storage in memory.
    pa_sms_Protocol_t   protocol,   ///< [IN] The protocol used for this message
    pa_sms_Storage_t    storage,    ///< [IN] SMS Storage used
    pa_sms_Pdu_t*       msgPtr      ///< [OUT] The message.
)
{
    SmsMsgInMemory * smsMsgPtr = GetSmsMsg(storage, index);

    if (NULL == smsMsgPtr)
    {
        LE_ERROR("Trying to access invalid SMS storage storage[%u] index[%u]", storage, index);
        return LE_NOT_POSSIBLE;
    }

    if (LE_SMS_STATUS_UNKNOWN == smsMsgPtr->pduContent.status)
    {
        return LE_NOT_POSSIBLE;
    }

    memcpy(msgPtr, &(smsMsgPtr->pduContent), sizeof(pa_sms_Pdu_t));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the indexes of messages stored in the preferred memory for a specific
 * status.
 *
 * @return LE_NOT_POSSIBLE   The function failed to get the indexes of messages stored in the
 *                           preferred memory.
 * @return LE_TIMEOUT        No response was received from the Modem.
 * @return LE_OK             The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ListMsgFromMem
(
    le_sms_Status_t     status,     ///< [IN] The status of message in memory.
    pa_sms_Protocol_t   protocol,   ///< [IN] The protocol to read
    uint32_t           *numPtr,     ///< [OUT] The number of indexes retrieved.
    uint32_t           *idxPtr,     ///< [OUT] The pointer to an array of indexes.
                                    ///        The array is filled with 'num' index values.
    pa_sms_Storage_t    storage     ///< [IN] SMS Storage used
)
{
    *numPtr = 0;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes one specific Message from preferred message storage.
 *
 * @return LE_NOT_POSSIBLE   The function failed to delete one specific Message from preferred
 *                           message storage.
 * @return LE_TIMEOUT        No response was received from the Modem.
 * @return LE_OK             The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_DelMsgFromMem
(
    uint32_t            index,    ///< [IN] Index of the message to be deleted.
    pa_sms_Protocol_t   protocol, ///< [IN] protocol
    pa_sms_Storage_t    storage   ///< [IN] SMS Storage used
)
{
    SmsMsgInMemory * smsMsgPtr = GetSmsMsg(storage, index);

    LE_DEBUG("Deleting message storage[%u] index[%u]", storage, index);

    if (NULL == smsMsgPtr)
    {
        return LE_NOT_POSSIBLE;
    }

    smsMsgPtr->pduContent.status = LE_SMS_STATUS_UNKNOWN;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes all Messages from preferred message storage.
 *
 * @return LE_NOT_POSSIBLE The function failed to delete all Messages from preferred message storage.
 * @return LE_COMM_ERROR   The communication device has returned an error.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_DelAllMsg
(
    void
)
{
    pa_sms_Storage_t storage;
    uint32_t idx;
    SmsMsgInMemory * smsMsgPtr;

    for(storage = PA_SMS_STORAGE_NV; storage <= PA_SMS_STORAGE_SIM; storage++)
    {
        for(idx = 0; idx < PA_SMS_SIMU_MAX_MSG_IN_MEM; idx++)
        {
            smsMsgPtr = GetSmsMsg(storage, idx);
            LE_ASSERT(smsMsgPtr != NULL);
            smsMsgPtr->pduContent.status = LE_SMS_STATUS_UNKNOWN;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function changes the message status.
 *
 * @return LE_NOT_POSSIBLE The function failed.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ChangeMessageStatus
(
    uint32_t            index,    ///< [IN] Index of the message to be deleted.
    pa_sms_Protocol_t   protocol, ///< [IN] protocol
    le_sms_Status_t     status,   ///< [IN] The status of message in memory.
    pa_sms_Storage_t    storage   ///< [IN] SMS Storage used
)
{
    SmsMsgInMemory * smsMsgPtr = GetSmsMsg(storage, index);

    if (NULL == smsMsgPtr)
    {
        return LE_NOT_POSSIBLE;
    }

    LE_DEBUG("Changing message status storage[%u] index[%u] status [%u] -> [%u]",
        storage, index, smsMsgPtr->pduContent.status, status);

    smsMsgPtr->pduContent.status = status;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the SMS center.
 *
 * @return LE_NOT_POSSIBLE The function failed.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_GetSmsc
(
    char*        smscPtr,  ///< [OUT] The Short message service center string.
    size_t       len       ///< [IN] The length of SMSC string.
)
{
    return le_utf8_Copy(smscPtr, SmsSmsc, len, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the SMS center.
 *
 * @return LE_NOT_POSSIBLE The function failed.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetSmsc
(
    const char*    smscPtr  ///< [IN] The Short message service center.
)
{
    return le_utf8_Copy(SmsSmsc, smscPtr, sizeof(SmsSmsc), NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * De-allocate a message if it's not used anymore
 */
//--------------------------------------------------------------------------------------------------
static void SmsMemPoolDestructor
(
    void* objPtr    ///< see parameter documentation in comment above.
)
{
    SmsMsgRef * smsMsgRefPtr = (SmsMsgRef *)objPtr;
    SmsMsgInMemory * smsMsgPtr = GetSmsMsg(smsMsgRefPtr->storage, smsMsgRefPtr->index);

    LE_ASSERT(smsMsgPtr != NULL);

    smsMsgPtr->pduContent.status = LE_SMS_FORMAT_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function handle messages originating from the simulated world.
 *
 * @return LE_NO_MEMORY    There is no more memory available to handle this message.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SmsServerHandleRemoteMessage
(
    pa_sms_SimuPdu_t * sourceMsgPtr
)
{
    int idx;
    SmsMsgInMemory * messageMemPtr = NULL; // Message stored in memory
    SmsMsgRef * smsMsgRefPtr;
    pa_sms_Storage_t storage = GetCurrentIncomingStorage();

    /* Find free spot in memory & allocate */
    for(idx = 0; idx < PA_SMS_SIMU_MAX_MSG_IN_MEM; idx++)
    {
        messageMemPtr = GetSmsMsg(storage, idx);

        if(messageMemPtr->pduContent.status == LE_SMS_STATUS_UNKNOWN)
        {
            messageMemPtr->pduContent.status = LE_SMS_RX_UNREAD;
            break;
        }
    }

    if(idx == PA_SMS_SIMU_MAX_MSG_IN_MEM)
    {
        LE_WARN("No more spot available in memory to store this message.");
        return LE_NO_MEMORY;
    }

    /* Create a ref to hold the index */
    smsMsgRefPtr = le_mem_ForceAlloc(SmsMemPoolRef);
    smsMsgRefPtr->index = idx;
    smsMsgRefPtr->storage = storage;

    LE_DEBUG("New message at storage[%u] idx[%d] (%p)", storage, idx, messageMemPtr);

    /* Store message */
    messageMemPtr->pduContent.dataLen = sourceMsgPtr->dataLen;
    memcpy(messageMemPtr->pduContent.data, sourceMsgPtr->data, sourceMsgPtr->dataLen);
    messageMemPtr->pduContent.protocol = sourceMsgPtr->protocol;

    /* Report index */    // Init the data for the event report
    pa_sms_NewMessageIndication_t msgIndication = {0};
    msgIndication.msgIndex = idx;
    msgIndication.storage = storage;
    msgIndication.protocol = sourceMsgPtr->protocol;

    le_event_Report(EventNewSmsId, &msgIndication, sizeof(msgIndication));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function handle messages originating from the Legato world.
 *
 * @return LE_NO_MEMORY    There is no more memory available to handle this message.
 * @return LE_NOT_POSSIBLE There was an error when handling the message.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SmsServerHandleLocalMessage
(
    pa_sms_SimuPdu_t * sourceMsgPtr
)
{
    int connIdx;
    ssize_t writeSz, expectedWriteSz;

    /* Deliver message to each connection */
    for(connIdx = 0; connIdx < PA_SMS_SIMU_MAX_CONN; connIdx++)
    {
        int connFd = SmsServerConnections[connIdx].fd;
        if(!SmsServerConnections[connIdx].used)
            continue;

        expectedWriteSz = sizeof(pa_sms_SimuPdu_t) + sourceMsgPtr->dataLen;
        writeSz = send(connFd, sourceMsgPtr, expectedWriteSz, 0);
        if(writeSz != expectedWriteSz)
        {
            LE_ERROR("Error while send message to fd=%d", connFd);
        }
    }

    /* Deliver message locally if necessary */
    {
        pa_sms_Message_t decodedMessage;
        le_result_t res;
        char localNumber[LE_MDMDEFS_PHONE_NUM_MAX_LEN];

        res = pa_sim_GetSubscriberPhoneNumber(localNumber, LE_MDMDEFS_PHONE_NUM_MAX_LEN);
        if(res != LE_OK)
        {
            LE_ERROR("Unable to get subscriber phone number.");
            return LE_NOT_POSSIBLE;
        }

        res = smsPdu_Decode(sourceMsgPtr->protocol, sourceMsgPtr->data, sourceMsgPtr->dataLen, &decodedMessage);
        if(res != LE_OK)
        {
            LE_ERROR("Unable to decode message.");
            return LE_NOT_POSSIBLE;
        }

        if(decodedMessage.type != PA_SMS_SMS_SUBMIT)
        {
            LE_ERROR("Unexpected type of PDU message.");
            return LE_NOT_POSSIBLE;
        }

        /* Destination and local number are the same */
        if( strncmp(decodedMessage.smsSubmit.da, localNumber, LE_MDMDEFS_PHONE_NUM_MAX_LEN) == 0)
        {
            pa_sms_Pdu_t pdu;
            le_result_t res;
            smsPdu_Encoding_t encoding;

            union {
                pa_sms_SimuPdu_t header;
                char buffer[1024];
            } txBuffer;

            switch(decodedMessage.smsSubmit.format)
            {
                case LE_SMS_FORMAT_BINARY:
                case LE_SMS_FORMAT_PDU:
                    encoding = SMSPDU_8_BITS;
                    break;

                case LE_SMS_FORMAT_TEXT:
                    encoding = SMSPDU_7_BITS;
                    break;

                default:
                    LE_ERROR("Unexpected format");
                    return LE_NOT_POSSIBLE;
            }

            LE_DEBUG("Sending message to self: len[%u] da[%s] format[%d] encoding[%d] protocol[%u]",
                    decodedMessage.smsSubmit.dataLen,
                    decodedMessage.smsSubmit.da,
                    decodedMessage.smsSubmit.format,
                    encoding,
                    sourceMsgPtr->protocol);

            res = smsPdu_Encode(
                    sourceMsgPtr->protocol,
                    decodedMessage.smsSubmit.data,
                    decodedMessage.smsSubmit.dataLen,
                    decodedMessage.smsSubmit.da,
                    encoding,
                    PA_SMS_SMS_DELIVER,
                    &pdu);

            if(res != LE_OK)
            {
                LE_ERROR("Unable to encode message.");
                return LE_NOT_POSSIBLE;
            }

            txBuffer.header.protocol = sourceMsgPtr->protocol;
            strcpy( (char *)txBuffer.header.origAddress, localNumber);
            strcpy( (char *)txBuffer.header.destAddress, localNumber);

            txBuffer.header.dataLen = pdu.dataLen;
            memcpy(txBuffer.header.data, pdu.data, pdu.dataLen);

            SmsServerHandleRemoteMessage( &(txBuffer.header) );
        }
        else
        {
            LE_DEBUG("Message not sent to self (='%s')", localNumber);
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a message incoming on a socket connection.
 */
//--------------------------------------------------------------------------------------------------
static void SmsServerRead
(
    int connFd,
    short events
)
{
    LE_ASSERT(events == POLLIN);

    ssize_t readSz;
    union {
        pa_sms_SimuPdu_t header;
        char buffer[1024];
    } rxBuffer;

    LE_INFO("Read (connFd=%d)", connFd);

    readSz = recv(connFd, rxBuffer.buffer, sizeof(rxBuffer.buffer), 0);
    LE_FATAL_IF( (readSz < 0), "Error on reception");

    if(readSz == 0)
    {
        int fdIndex;

        LE_INFO("Client has disconnected (fd=%d)", connFd);

        /* Find connection record */
        for(fdIndex = 0; fdIndex < PA_SMS_SIMU_MAX_CONN; fdIndex++)
        {
            if(SmsServerConnections[fdIndex].fd == connFd)
            {
                LE_DEBUG("Releasing connection idx=%d fd=%d", fdIndex, connFd);

                le_fdMonitor_Delete(SmsServerConnections[fdIndex].fdMonitorRef);

                // Close the connection.
                int result;
                do
                {
                    result = close(connFd);
                } while ((result == -1) && (errno == EINTR));
                LE_CRIT_IF(result == -1, "close() failed for connection fd %d. Errno %m.", connFd);

                // Clear out the connection record.
                SmsServerConnections[fdIndex].fd = -1;
                SmsServerConnections[fdIndex].fdMonitorRef = NULL;
                SmsServerConnections[fdIndex].used = false;

                break;
            }
        }

        LE_FATAL_IF( (fdIndex == PA_SMS_SIMU_MAX_CONN), "Connection not found" );

        return;
    }

    LE_FATAL_IF( (readSz < sizeof(pa_sms_SimuPdu_t)), "Received size < size of header" );

    LE_INFO("Received message from '%s', to '%s' (len=%u, readSz=%zd)",
        rxBuffer.header.origAddress,
        rxBuffer.header.destAddress,
        rxBuffer.header.dataLen,
        readSz);

    if(!mrc_simu_IsOnline())
    {
        LE_WARN("Not handling message because we're offline.");
        return;
    }

    /* Not handling multiple message per read */
    LE_FATAL_IF( (sizeof(pa_sms_SimuPdu_t) + rxBuffer.header.dataLen) != readSz, "Problem on reception (size=%zd)", readSz);

    SmsServerHandleRemoteMessage( &(rxBuffer.header) );
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle an incoming socket connection.
 */
//--------------------------------------------------------------------------------------------------
static void SmsServerConn
(
    int listenFd
)
{
    int connFd;
    struct sockaddr inAddr;
    socklen_t inLen = sizeof(struct sockaddr);
    int fdIndex = -1;
    char monitorFdName[100];

    LE_INFO("Conn listenFd=%d", listenFd);

    connFd = accept(listenFd, &inAddr, &inLen);
    LE_FATAL_IF( (connFd < 0), "Unable to accept connection" );

    /* Look for a free connection */
    for(fdIndex = 0; fdIndex < PA_SMS_SIMU_MAX_CONN; fdIndex++)
    {
        if(SmsServerConnections[fdIndex].used == false)
        {
            LE_INFO("Accept Connection idx=%d fd=%d", fdIndex, connFd);
            break;
        }
    }

    if(fdIndex == PA_SMS_SIMU_MAX_CONN)
    {
        LE_WARN("Nb of allowed connections reached (%d)", PA_SMS_SIMU_MAX_CONN);
    }

    snprintf(monitorFdName, sizeof(monitorFdName), "SmsSimuConn[%u]", fdIndex);

    SmsServerConnections[fdIndex].used = true;
    SmsServerConnections[fdIndex].fd = connFd;
    SmsServerConnections[fdIndex].fdMonitorRef = le_fdMonitor_Create(monitorFdName,
                                                                     connFd,
                                                                     SmsServerRead,
                                                                     POLLIN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle an error on the socket connection.
 */
//--------------------------------------------------------------------------------------------------
static void SmsServerError
(
    void
)
{
    LE_FATAL("SMS Server Error");
}

//--------------------------------------------------------------------------------------------------
/**
 * Event handler for file descriptor events for the SMS server listen socket.
 **/
//--------------------------------------------------------------------------------------------------
static void SmsServerListenEvent
(
    int fd,         ///< Socket file descriptor.
    short events    ///< Bit map of events.  Expect POLLIN (readable) and POLLERR (error).
)
//--------------------------------------------------------------------------------------------------
{
    if (events & POLLERR)
    {
        SmsServerError();
    }

    if (events & POLLIN)
    {
        SmsServerConn(fd);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize SMS server
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InitSmsServer
(
    uint16_t port   ///< [IN] TCP Port on which the server is provided
)
{
    struct sockaddr_in sockAddr;
    int res;

    SmsServerListenFd = socket(AF_INET, SOCK_STREAM, 0);
    LE_FATAL_IF( (SmsServerListenFd < 0), "Error when creating socket ..." );

    bzero(&sockAddr, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(port);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    res = bind(SmsServerListenFd, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
    LE_FATAL_IF( (res < 0), "Error when binding socket ..." );

    LE_INFO("SMS Server on port %u (listenFd=%d)", port, SmsServerListenFd);

    res = listen(SmsServerListenFd, 1024);
    LE_FATAL_IF( (res < 0), "Error when starting to listen on socket ..." );

    SmsServerMonitorRef = le_fdMonitor_Create("SmsSimuFd",
                                              SmsServerListenFd,
                                              SmsServerListenEvent,
                                              POLLIN);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * SMS Stub initialization.
 *
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t sms_simu_Init
(
    void
)
{
    LE_INFO("PA SMS Init");

    EventNewSmsId = le_event_CreateId("EventNewSmsId", sizeof(pa_sms_NewMessageIndication_t));

    pa_sms_DelAllMsg();

    SmsMemPoolRef = le_mem_CreatePool("SmsMemPoolRef", sizeof(SmsMsgRef));
    le_mem_SetDestructor(SmsMemPoolRef, SmsMemPoolDestructor);

    InitSmsServer(5000);

    return LE_OK;
}

