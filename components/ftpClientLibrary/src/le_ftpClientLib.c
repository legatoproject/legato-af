/**
 * @file le_ftpClientLib.c
 *
 * Implementation of Legato FTP client built on top of le_socketLib.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#include "legato.h"
#include "interfaces.h"
#include "../le_ftpClient.h"
#include "le_socketLib.h"

#ifndef LE_CONFIG_FTPCLIENT_SESSION_MAX
#define LE_CONFIG_FTPCLIENT_SESSION_MAX 6      ///< Maximum session number.
#endif
#ifndef LE_CONFIG_FTPCLIENT_SERVER_NAME_MAX
#define LE_CONFIG_FTPCLIENT_SERVER_NAME_MAX 64 ///< Server hostname/address.
#endif
#ifndef LE_CONFIG_FTPCLIENT_USER_NAME_MAX
#define LE_CONFIG_FTPCLIENT_USER_NAME_MAX 32   ///< User name.
#endif
#ifndef LE_CONFIG_FTPCLIENT_PASSWORD_MAX
#define LE_CONFIG_FTPCLIENT_PASSWORD_MAX 32    ///< User's password.
#endif
#ifndef LE_CONFIG_FTPCLIENT_BUFFER_SIZE
#define LE_CONFIG_FTPCLIENT_BUFFER_SIZE 256    ///< Data buffer size
#endif

/// FTP  possible response code
#define RESP_LOGGED_IN           230
#define RESP_SERVER_READY        220
#define RESP_AUTH_OK             234
#define RESP_USER_OK             331
#define RESP_USER_LOGGED_IN      232
#define RESP_COMMAND_OK          200
#define RESP_PASV_PASSIVE        227
#define RESP_EPSV_PASSIVE        229
#define RESP_REST_AT             350
#define RESP_OPENING_DC          150
#define RESP_DC_OPENED           125
#define RESP_NO_FILE             550
#define RESP_TRANS_OK            226
#define RESP_QUIT_OK             221
#define RESP_SIZE_OK             213
#define RESP_DELE_OK             250


#define RESP_INVALID             -1

/// FTP response timeout
#define FTP_TIMEOUT_MS           5000
/// FTP response buffer size
#define FTP_RESP_MAX_SIZE        513
/// FTP data buffer size
#define FTP_DATA_MAX_SIZE        1025

//--------------------------------------------------------------------------------------------------
/**
 * Calculate minimum of two values.
 *
 * @param   a   First value.
 * @param   b   Second value.
 *
 * @return Smallest value.
 */
//--------------------------------------------------------------------------------------------------
#ifndef MIN
#  define MIN(a, b)   ((a) < (b) ? (a) : (b))
#endif

//--------------------------------------------------------------------------------------------------
/**
 *  Security mode.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_FTP_CLIENT_NONSECURE,    ///< Non-secure FTP
    LE_FTP_CLIENT_SECURE        ///< Explicit FTPS
} le_ftpClient_SecurityMode_t;

//--------------------------------------------------------------------------------------------------
/**
 *  FTP operations.
 */
//--------------------------------------------------------------------------------------------------
enum Operation
{
    OP_NONE,        ///< No current operation.
    OP_CONNECT,     ///< Connect to server.
    OP_DISCONNECT,  ///< Disconnect from server.
    OP_STORE,       ///< Upload a file to the server.
    OP_RETRIEVE,    ///< Download a file from the server.
    OP_SIZE,        ///< Get the size of a remote file.
    OP_DELETE       ///< Delete a remote file.
};

//--------------------------------------------------------------------------------------------------
/**
 * Enum for FTP client state machine
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    FTP_CLOSED=0,   ///< Client is closed.
    FTP_CONNECTED,  ///< Connected to server.
    FTP_AUTH_SENT,  ///< AUTH command is sent
    FTP_TLS_HSHAKE, ///< TLS Handshake complete
    FTP_USER_SENT,  ///< user name is sent to server.
    FTP_PASS_SENT,  ///< password is sent to server.
    FTP_PBSZ_SENT,  ///< PBSZ command is sent.
    FTP_PROT_SENT,  ///< PROT command is sent.
    FTP_LOGGED,     ///< Logged into server with specfied user/pwd.
    FTP_TYPE_SENT,  ///< TYPE command is sent.
    FTP_PASV_SENT,  ///< PASV command is sent.
    FTP_RETR_SENT,  ///< RETR command is sent.
    FTP_STOR_SENT,  ///< STOR command is sent.
    FTP_XFERING,    ///< Under data transfering.
    FTP_DATAEND,    ///< Data transfering is completed.
    FTP_QUIT,       ///< Start to send QUIT command.
    FTP_QUIT_SENT,  ///< QUIT command is sent to server.
    FTP_CLOSING,    ///< Under FTP client closing.
    FTP_DELE_SENT,  ///< DELE command is sent.
    FTP_SIZE_SENT,  ///< SIZE command is sent.
    FTP_REST_SENT,  ///< REST command is sent.
    FTP_APPE_SENT,  ///< APPE command is sent.
    FTP_XFEREND     ///< Response of data transfering completion is received.
}
FtpSessionState_t;

//--------------------------------------------------------------------------------------------------
/**
 *  FTP client session.
 */
//--------------------------------------------------------------------------------------------------
struct le_ftpClient_Session
{
    char    srcIpAddr[LE_MDC_IPV6_ADDR_MAX_BYTES];          ///< Source IP address
    char    serverStr[LE_CONFIG_FTPCLIENT_SERVER_NAME_MAX]; ///< Server hostname/address.
    char    userStr[LE_CONFIG_FTPCLIENT_USER_NAME_MAX];     ///< User name.
    char    passwordStr[LE_CONFIG_FTPCLIENT_PASSWORD_MAX];  ///< User's password.
    char    dsAddrStr[LE_CONFIG_FTPCLIENT_SERVER_NAME_MAX]; ///< Data session address.
    uint16_t dsPort;                                        ///< Data session port.
    uint16_t serverPort;                                    ///< Server port.
    enum Operation               operation;                 ///< Current operation.
    le_ftpClient_SecurityMode_t  securityMode;              ///< Security mode
    union
    {
        le_ftpClient_WriteFunc_t writeFunc;                 ///< Callback to write downloaded data.
        uint64_t                 fileSize;                  ///< Size of remote file.
    };
    void                         *userDataPtr;              ///< User data passed to writeFunc.
    uint8_t                      cipherIdx;                 ///< Cipher suites index.
    const uint8_t                *certPtr;                  ///< Pointer to certificate
    size_t                       certSize;                  ///< Certificate size in bytes
    bool                         isConnected;               ///< Connection status.
    int                          ipAddrFamily;              ///< AF_INET (IPV4) or AF_INET6 (IPV6)
    le_result_t                  result;                    ///< Result of the current operation.
    uint32_t                     timeout;                   ///< Default timeout in milliseconds.
    le_ftpClient_EventFunc_t     eventHandlerFunc;          ///< Asynchronous event handler.
    void                         *eventHandlerDataPtr;      ///< User data pass to event handler.
    le_socket_Ref_t              ctrlSocketRef;             ///< Safe reference to control socket.
    le_socket_Ref_t              dataSocketRef;             ///< Safe reference to data socket.
    le_timer_Ref_t               timerRef;                  ///< FTP client connection timer.
    uint64_t                     restOffSet;                ///< offset for REST command.
    bool                         recvDone;                  ///< data session is disconnected.
    const char                   *remotePath;               ///< File path on remote server.
    int                          response;                  ///< Response code of last request.
    FtpSessionState_t            controlState;              ///< FTP client current state.
    FtpSessionState_t            targetState;               ///< FTP client next state.
};

//--------------------------------------------------------------------------------------------------
/**
 *  Memory pool for FTP client sessions.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SessionPool;
LE_MEM_DEFINE_STATIC_POOL(Session, LE_CONFIG_FTPCLIENT_SESSION_MAX,
    sizeof(struct le_ftpClient_Session));

//--------------------------------------------------------------------------------------------------
/**
 *  Determine if an operation is of blocking type or not.
 *
 *  @return True if the operation blocks.
 */
//--------------------------------------------------------------------------------------------------
static bool IsBlocking
(
    enum Operation op ///< Operation.
)
{
    switch (op)
    {
        case OP_SIZE:
        case OP_CONNECT:
        case OP_DISCONNECT:
        case OP_DELETE:
            return true;
        default:
            return false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Force to close the specific FTP client session
 */
//--------------------------------------------------------------------------------------------------
static void FtpClientClose
(
    le_ftpClient_SessionRef_t sessionRef   ///< [IN] ftp client session ref
)
{
    LE_FATAL_IF(sessionRef == NULL, "FTP client session is NULL.");

    if (sessionRef->ctrlSocketRef != NULL)
    {
        le_socket_Disconnect(sessionRef->ctrlSocketRef);
        le_socket_Delete(sessionRef->ctrlSocketRef);
        sessionRef->ctrlSocketRef = NULL;
    }

    if (sessionRef->dataSocketRef != NULL)
    {
        le_socket_Disconnect(sessionRef->dataSocketRef);
        le_socket_Delete(sessionRef->dataSocketRef);
        sessionRef->dataSocketRef = NULL;
    }

    if (sessionRef->timerRef != NULL)
    {
        le_timer_Delete(sessionRef->timerRef);
        sessionRef->timerRef = NULL;
    }

    if (sessionRef->certPtr != NULL)
    {
        le_mem_Release((void *)sessionRef->certPtr);
        sessionRef->certPtr = NULL;
    }

    sessionRef->controlState = FTP_CLOSED;
    sessionRef->isConnected = false;
    LE_INFO("client session (%p) closed.", sessionRef);
}

static void TimeoutHandler
(
    le_timer_Ref_t timerRef    ///< [IN] Expired timer reference
)
{
    le_ftpClient_SessionRef_t contextPtr = le_timer_GetContextPtr(timerRef);
    le_ftpClient_Event_t callbackType;

    if (contextPtr == NULL)
    {
        LE_FATAL("ContextPtr of timer %p is not found.", timerRef);
    }

    if (IsBlocking(contextPtr->operation))
    {
        LE_FATAL("Not in async operations.");
    }

    LE_WARN("Timeout when waiting for async response from remote FTP server");
    // Force to close client session.
    FtpClientClose(contextPtr);
    contextPtr->result = LE_TIMEOUT;
    if (contextPtr->operation == OP_NONE)
    {
        callbackType =  LE_FTP_CLIENT_EVENT_TIMEOUT;
    }
    else
    {
        callbackType = LE_FTP_CLIENT_EVENT_ERROR;
    }

    contextPtr->operation = OP_NONE;
    // Call user defined callback function.
    if (contextPtr->eventHandlerFunc != NULL)
    {
        contextPtr->eventHandlerFunc(
            contextPtr,
            callbackType,
            contextPtr->result,
            contextPtr->eventHandlerDataPtr
        );
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send standard request message to remote FTP server.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SendRequestMessage
(
   le_ftpClient_SessionRef_t sessionRef,  ///< [IN] ftp client session ref
   char* msgPtr,                          ///< [IN] message buffer
   size_t len                             ///< [IN] message length
)
{
    LE_FATAL_IF(sessionRef == NULL, "sessionRef is NULL.");
    LE_FATAL_IF(msgPtr == NULL, "msgPtr is NULL.");
    LE_FATAL_IF(len == 0, "msgPtr len is 0");

    if ((sessionRef->ctrlSocketRef == NULL) ||
        (sessionRef->controlState == FTP_CLOSED))
    {
        return LE_NOT_PERMITTED;
    }

    LE_INFO("FTP session(%p) in op(%d) state(%d) request message: %s",
            sessionRef, sessionRef->operation, sessionRef->controlState, msgPtr);

    return le_socket_Send(sessionRef->ctrlSocketRef, msgPtr, len);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Get a full FTP response message.
 *  This function handles both single-line response and multi-lines response.
 *
 *  Note : Since it's possible to receive two different response messages like 150
 *  and 226 in one le_socket_Read() call, here we can return two response messages
 *  if exists.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReceiveResponseMessage
(
    le_ftpClient_SessionRef_t sessionRef, ///< [IN] ftp client session ref.
    char* responseLine1Ptr,               ///< [OUT] hold the last line of the first response
    size_t len1,                          ///< [IN] buffer length of responseLine1Ptr.
    char* responseLine2Ptr,               ///< [OUT] hold the last line of the second response
                                          ///        if exists, this can be NULL.
    size_t len2                           ///< [IN] buffer length of responseLine2Ptr. this is
                                          ///       ignored if responseLine2Ptr is NULL.
)
{
    char buffer[FTP_RESP_MAX_SIZE] = {0};
    char* data = NULL;
    char* p = NULL;
    size_t length, savedBytes;
    bool recvDone = false;
    bool pendingRead = false;
    int respCnt = 0;
    int respCode = 0;

    LE_FATAL_IF(sessionRef == NULL, "sessionRef is NULL.");
    LE_FATAL_IF(responseLine1Ptr == NULL, "responseLine1Ptr is NULL.");
    LE_FATAL_IF(len1 == 0, "len1 is 0.");

    if (responseLine2Ptr != NULL)
    {
        LE_FATAL_IF(len2 == 0, "len2 is 0 while responseLine2Ptr is not NULL.");
    }

    if ((sessionRef->ctrlSocketRef == NULL) ||
        (sessionRef->controlState == FTP_CLOSED))
    {
        return LE_NOT_PERMITTED;
    }

    data = buffer;
    length = sizeof(buffer) - 1;
    savedBytes = 0;
    do
    {
        if (LE_OK != le_socket_Read(sessionRef->ctrlSocketRef, buffer+savedBytes, &length))
        {
            LE_ERROR("Error receiving data.");
            return LE_FAULT;
        }

        if (length == 0)
        {
            LE_ERROR("Session is closed by remote server.");
            return LE_CLOSED;
        }

        do
        {
            p = strstr(data,"\r\n");
            if(p != NULL)
            {
                // Getting a response line, save current line into 'data'.
                *p = '\0';
                LE_INFO("FTP session(%p) in op(%d) state(%d) response line: %s",
                        sessionRef, sessionRef->operation, sessionRef->controlState, data);

                // Checking if single-line response or multi-lines response.
                // FTP_SingleLineResponse = 3DIGIT <SP> * <CR><LF>
                // FTP_MultiLineResponse = 3DIGIT- * <CR><LF>
                //                         *<CR><LF>
                //                         FTP_SingleLineResponse
                if (*(data+3) == ' ')
                {
                    // Checking 3DIGIT.
                    sscanf(data, "%d", &respCode);
                    if (respCode < 100 || respCode > 999)
                    {
                        LE_ERROR("Invalid response code (%d).", respCode);
                        return LE_FAULT;
                    }

                    recvDone = true;
                    pendingRead = false;
                    LE_INFO("response message %d received.", ++respCnt );
                    if (respCnt == 1)
                    {
                        // Copy the last line of the first response message.
                        if (le_utf8_Copy(responseLine1Ptr, data, len1, NULL) != LE_OK)
                        {
                            LE_ERROR("Failed to copy response message %d.", respCnt);
                            return LE_FAULT;
                        }
                    }
                    else if ((respCnt == 2) && (responseLine2Ptr != NULL))
                    {
                        // Copy the last line of the second response message.
                        if (le_utf8_Copy(responseLine2Ptr, data, len2, NULL) != LE_OK)
                        {
                            LE_ERROR("Failed to copy response message %d.", respCnt);
                            return LE_FAULT;
                        }
                    }
                }
                else
                {
                    // It's not the final response line, just set the pendingRead flag.
                    pendingRead = true;
                }

                // Move 'data' to the begining of next response line.
                data = p + 2;
            }
            else
            {
                pendingRead = true;
                break;
            }
        }
        while(strlen(data) > 0);

        if (recvDone && !pendingRead)
        {
            return LE_OK;
        }

        savedBytes += length;
        length = sizeof(buffer) - 1 - savedBytes;
        LE_INFO("%d bytes processed, waiting for more bytes...", savedBytes);
    }
    while(length > 0);

    LE_ERROR("Failed to read a valid response message.");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Try to receive a valid response message and retrieve the response code from message.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FtpClientGetResponseCode
(
    le_ftpClient_SessionRef_t sessionRef, ///< [IN] ftp client session ref
    int* responsePtr                      ///< [OUT] response code.
)
{
    char buffer[FTP_RESP_MAX_SIZE] = {0};
    le_result_t status;
    int response = RESP_INVALID;

    LE_FATAL_IF(responsePtr == NULL, "responsePtr is NULL.");

    status = ReceiveResponseMessage(sessionRef, buffer, FTP_RESP_MAX_SIZE, NULL, 0);
    if (LE_OK == status)
    {
        sscanf(buffer, "%d", &response);
        if (response != RESP_INVALID)
        {
            // Valid response code found.
            *responsePtr = response;
            return LE_OK;
        }
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Try to receive multi response messages and retrieve related response code.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FtpClientGetMultiResponseCode
(
    le_ftpClient_SessionRef_t sessionRef, ///< [IN] ftp client session ref
    int* response1Ptr,                      ///< [OUT] response code of first reponse message.
    int* response2Ptr                       ///< [OUT] response code of second reponse message
                                            /// if exists.
)
{
    char buffer1[FTP_RESP_MAX_SIZE] = {0};
    char buffer2[FTP_RESP_MAX_SIZE] = {0};
    le_result_t status;
    int response = RESP_INVALID;

    LE_FATAL_IF(response1Ptr == NULL, "response1Ptr is NULL.");
    LE_FATAL_IF(response2Ptr == NULL, "response2Ptr is NULL.");

    status = ReceiveResponseMessage(sessionRef, buffer1, sizeof(buffer1), buffer2, sizeof(buffer2));
    if (LE_OK == status)
    {
        sscanf(buffer1, "%d", &response);
        if (response != RESP_INVALID)
        {
            // Valid response code found.
            *response1Ptr = response;

            response = RESP_INVALID;
            if (strlen(buffer2)>0)
            {
                // Get the second response code if exists.
                sscanf(buffer2, "%d", &response);
            }
            *response2Ptr = response;
            return LE_OK;
        }
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Try to receive a response message and retrieve the File Size.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FtpClientGetFileSize
(
    le_ftpClient_SessionRef_t sessionRef, ///< [IN] ftp client session ref
    uint64_t* SizePtr                      ///< [OUT] file size code.
)
{
    char buffer[FTP_RESP_MAX_SIZE] = {0};
    le_result_t status = LE_FAULT;
    int response = RESP_INVALID;
    uint64_t size = (uint64_t)RESP_INVALID;

    status = ReceiveResponseMessage(sessionRef, buffer, FTP_RESP_MAX_SIZE, NULL, 0);
    if (LE_OK == status)
    {
        sscanf(buffer, "%d", &response);
        sessionRef->response = response;
        if (response != RESP_INVALID)
        {
            if (response == RESP_SIZE_OK)
            {
                // 213<SP><size>,find the file size
                sscanf(buffer+4, "%"PRIu64, &size);
                *SizePtr = size;
                return LE_OK;
            }
            else if (response == RESP_NO_FILE)
            {
                LE_ERROR("No such file found in server.");
                return LE_NOT_FOUND;
            }
        }
    }
    else
    {
        sessionRef->response = RESP_INVALID;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Try to receive a response message and retrieve the IP and PORT from the message.
 *  PASV response: 227 Entering Passive Mode (129,240,118,47,203,104)
 *  EPSV response: 229 Entering Extended Passive Mode (|||53501|)
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FtpClientGetServerAddress
(
    le_ftpClient_SessionRef_t sessionRef, ///< [IN] ftp client session reference.
    char* serverPtr,                      ///< [OUT] the buffer to hold the server address.
    size_t len,                           ///< [IN] buffer length.
    uint16_t* portPtr                     ///< [OUT] port number.
)
{
    char buffer[FTP_RESP_MAX_SIZE] = {0};
    le_result_t status;
    int response = RESP_INVALID;
    char* ptr;
    char* ptr1;
    uint32_t a,b,c,d; // Server IP address string "a.b.c.d"
    uint16_t ph, pl;  // port

    LE_FATAL_IF(serverPtr == NULL, "serverPtr is NULL.");
    LE_FATAL_IF(portPtr == NULL, "portPtr is NULL.");
    LE_FATAL_IF(len == 0, "len is 0.");

    status = ReceiveResponseMessage(sessionRef, buffer, FTP_RESP_MAX_SIZE, NULL, 0);
    if (LE_OK == status)
    {
        sscanf(buffer, "%d", &response);

        // Find server connection parameter
        ptr = strchr(buffer, '(');
        ptr1 = strchr(buffer, ')');
        if (ptr == NULL || ptr1 == NULL)
        {
            LE_ERROR("Invalid EPSV response.");
            return LE_NOT_FOUND;
        }

        ptr1 = '\0';
        ptr += 1;
        if (response == RESP_PASV_PASSIVE)
        {
            if (sscanf(ptr, "%"SCNu32",%"SCNu32",%"SCNu32",%"SCNu32",%"SCNu16",%"SCNu16,
                        &a, &b, &c, &d, &ph, &pl) != 6)
            {
                LE_ERROR("Invalid address in EPSV response.");
                return LE_FAULT;
            }

            LE_FATAL_IF(snprintf(serverPtr, len, "%"PRIu32".%"PRIu32".%"PRIu32".%"PRIu32,
                            a, b, c, d) >= len,
                        "serverPtr buffer overflow.");

            *portPtr = (ph << 8) | (pl & 255);
            LE_INFO("Get data session address = %s, port = %hu", serverPtr, *portPtr);

            return LE_OK;
        }
        else if (response == RESP_EPSV_PASSIVE)
        {
            // No target ip in EPSV response, directly use server address.
            memcpy(serverPtr, sessionRef->serverStr, LE_CONFIG_FTPCLIENT_SERVER_NAME_MAX);
            for (++ptr; *ptr == '|'; ++ptr)
            {
                // do nothing
            }

            pl = strtoul(ptr, &ptr, 10);
            *portPtr = pl;
            LE_INFO("Getting data session address = %s, port = %hu", serverPtr, pl);
            return LE_OK;
        }
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Create FTP connection timer
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FtpClientCreateTimer
(
    le_ftpClient_SessionRef_t sessionRef   ///< [IN] ftp client session ref
)
{
    LE_FATAL_IF(sessionRef == NULL, "sessionRef is NULL.");

    if (sessionRef->timerRef != NULL)
    {
        LE_ERROR("FTP conection timer is already created.");
        return LE_FAULT;
    }

    sessionRef->timerRef = le_timer_Create("ConnectionTimer");
    if (NULL ==  sessionRef->timerRef)
    {
        LE_ERROR("Failed to create ConnectionTimer.");
        return LE_FAULT;
    }
    else
    {
        le_timer_SetRepeat(sessionRef->timerRef, 1);
        le_timer_SetContextPtr(sessionRef->timerRef, sessionRef);
        le_timer_SetHandler(sessionRef->timerRef, TimeoutHandler);
        le_timer_SetMsInterval(sessionRef->timerRef, sessionRef->timeout);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Disconnect the data port of the remote FTP server.
 */
//--------------------------------------------------------------------------------------------------
static void FtpClientDisconnectDataServer
(
    le_ftpClient_SessionRef_t sessionRef  ///< [IN] ftp client session ref
)
{
    LE_FATAL_IF(sessionRef == NULL, "FTP client session is NULL.");

    if (sessionRef->dataSocketRef != NULL)
    {
        le_socket_Disconnect(sessionRef->dataSocketRef);
        le_socket_Delete(sessionRef->dataSocketRef);
        sessionRef->dataSocketRef = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function process the async events on FTP data session socket
 */
//--------------------------------------------------------------------------------------------------
static void FtpClientDataHandler
(
    le_socket_Ref_t  socketRef,  ///< [IN] Socket context reference
    short            events,     ///< [IN] Bitmap of events that occurred
    void*            userPtr     ///< [IN] User data pointer
)
{
    char buffer[FTP_DATA_MAX_SIZE] = {0};
    const char* data = buffer;
    size_t length = sizeof(buffer) - 1;

    le_ftpClient_SessionRef_t  contextPtr = userPtr;

    LE_FATAL_IF(contextPtr == NULL, "FTP client session is NULL.");

    LE_INFO("Data handler: event = %d", events);

    if (contextPtr->timerRef != NULL)
    {
        le_timer_Restart(contextPtr->timerRef);
    }

    // Check if data session is disconnected.
    if (events & POLLRDHUP)
    {
        goto transfer_end;
    }

    // Call user writeFunc() for RETR/REST command.
    if (events & POLLIN)
    {
        if (contextPtr->operation == OP_RETRIEVE)
        {
            if (LE_OK == le_socket_Read(contextPtr->dataSocketRef, buffer, &length))
            {
                if (contextPtr->writeFunc != NULL)
                {
                    contextPtr->writeFunc(data, length, contextPtr->userDataPtr);
                }
                return;
            }
            else
            {
                // Session is closed by remote server.
                goto transfer_end;
            }
        }
    }

    if (events & POLLOUT)
    {
        return;
    }

transfer_end:
    contextPtr->recvDone = true;
    if (contextPtr->controlState == FTP_XFEREND)
    {
        LE_INFO("Data transfer done after XFEREND notification.");
        FtpClientDisconnectDataServer(contextPtr);
        contextPtr->controlState = FTP_LOGGED;
        contextPtr->operation = OP_NONE;
        contextPtr->result = LE_OK;

        // Call user defined callback function.
        if (contextPtr->eventHandlerFunc != NULL)
        {
            contextPtr->eventHandlerFunc(contextPtr,
                                         LE_FTP_CLIENT_EVENT_DATAEND,
                                         contextPtr->result,
                                         contextPtr->eventHandlerDataPtr);
        }
    }
    else if (contextPtr->securityMode == LE_FTP_CLIENT_SECURE)
    {
        LE_INFO("FTPS Server closed the connection.");
        FtpClientDisconnectDataServer(contextPtr);
    }
    else
    {
        LE_INFO("Data transfer done before XFEREND notification.");
    }
}

//-------------------------------------------------------------------------------------------------
/**
 *  Connect the data port of the remote FTP server for data transfer.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FtpClientConnectDataServer
(
    le_ftpClient_SessionRef_t sessionRef  ///< [IN] ftp client session ref
)
{
    LE_FATAL_IF(sessionRef == NULL, "FTP client session is NULL.");

    if (!sessionRef->isConnected)
    {
        return LE_NOT_PERMITTED;
    }

    LE_FATAL_IF(sessionRef->dataSocketRef != NULL,"data socket is already created!");

    // Create the data socket.
    sessionRef->dataSocketRef = le_socket_Create(sessionRef->dsAddrStr,
                                                 sessionRef->dsPort,
                                                 sessionRef->srcIpAddr, TCP_TYPE);
    if (NULL == sessionRef->dataSocketRef)
    {
        LE_ERROR("Failed to create data socket for server %s:%u.", sessionRef->dsAddrStr,
                 sessionRef->dsPort);
        return LE_FAULT;
    }

    // Set response timeout (5s).
    if (LE_OK != le_socket_SetTimeout(sessionRef->dataSocketRef, FTP_TIMEOUT_MS))
    {
       LE_ERROR("Failed to set response timeout.");
       goto freeSocket;
    }

    // Add a certificate for FTPS
    if (sessionRef->securityMode == LE_FTP_CLIENT_SECURE)
    {
        LE_INFO("Adding root CA certificates to data channel");
        if (le_socket_AddCertificate(sessionRef->dataSocketRef,
                                     sessionRef->certPtr,
                                     sessionRef->certSize) != LE_OK)
        {   LE_ERROR("Failed to add root CA certificates.");
            goto freeSocket;
        }

        LE_INFO("Setting cipher suites to data channel");
        if (le_socket_SetCipherSuites(sessionRef->dataSocketRef, sessionRef->cipherIdx) != LE_OK)
        {
            LE_ERROR("Failed to set cipher suites.");
            goto freeSocket;
        }
    }

    // Connect to the data port of the remote FTP server.
    if (LE_OK != le_socket_Connect(sessionRef->dataSocketRef))
    {
        LE_ERROR("Failed to connect data session %s:%u.", sessionRef->dsAddrStr,
                 sessionRef->dsPort);
        goto freeSocket;
    }

    // Set the socket event callback for fd monitor.
    if (LE_OK != le_socket_AddEventHandler(sessionRef->dataSocketRef,
                                               FtpClientDataHandler,sessionRef))
    {
        LE_ERROR("Failed to add data socket event handler.");
        goto freeSocket;
    }

    // Enable async mode and start fd monitor.
    if (LE_OK != le_socket_SetMonitoring(sessionRef->dataSocketRef, true))
    {
       LE_ERROR("Failed to enable data socket monitor.");
       goto freeSocket;
    }

    // Succeed to login server with specified usr/pwd.
    LE_INFO("Succeed to connect data server %s:%u.", sessionRef->dsAddrStr,
            sessionRef->dsPort);
    sessionRef->recvDone = false;

    return LE_OK;

freeSocket:
    if(sessionRef->dataSocketRef != NULL)
    {
        le_socket_Delete(sessionRef->dataSocketRef);
        sessionRef->dataSocketRef = NULL;
    }
    return LE_COMM_ERROR;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Destructor for FTP client session.  Closes the open connection, if any, and cleans up memory.
 */
//--------------------------------------------------------------------------------------------------
static void SessionDestructor
(
    le_ftpClient_SessionRef_t sessionRef    ///< Session to destroy.
)
{
    le_ftpClient_SetEventCallback(sessionRef, NULL, NULL);
    FtpClientClose(sessionRef);
    memset(sessionRef->passwordStr, 0, sizeof(sessionRef->passwordStr));
}

//--------------------------------------------------------------------------------------------------
/**
 *  Perform one-time initialization of the FTP client.
 */
//--------------------------------------------------------------------------------------------------
void InitFtpClientComponent
(
    void
)
{
    SessionPool = le_mem_InitStaticPool(Session, LE_CONFIG_FTPCLIENT_SESSION_MAX,
                    sizeof(struct le_ftpClient_Session));
    le_mem_SetDestructor(SessionPool, (le_mem_Destructor_t) &SessionDestructor);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function implements FTP client state machine.
 *
 * @note
 * - For asynchronous requests, this function is called by socket monitoring when data is available.
 * - For synchronous requests, this function is looped inside the function.
 */
//--------------------------------------------------------------------------------------------------
static void FtpClientStateMachine
(
    le_socket_Ref_t  socketRef,  ///< [IN] Socket context reference
    short            events,     ///< [IN] Bitmap of events that occurred
    void*            userPtr     ///< [IN] User data pointer
)
{
    le_result_t status = LE_OK;
    bool restartLoop = false;
    bool sessionClosed = false;
    int response = RESP_INVALID;
    int response1 = RESP_INVALID;
    char msgBuf[FTP_RESP_MAX_SIZE] = {0};
    int msgLen;
    le_ftpClient_Event_t callbackEvent;
    le_ftpClient_SessionRef_t  contextPtr = userPtr;

    LE_FATAL_IF(contextPtr == NULL, "FTP client session is NULL.");

    if ((!contextPtr->isConnected) &&
        (contextPtr->controlState != FTP_CONNECTED))
    {
        LE_WARN("Client is not connected.");
        return;
    }

    // Check if remote server closed connection
    if (events & POLLRDHUP)
    {
        LE_INFO("Connection closed by remote server");
        contextPtr->controlState = FTP_CLOSING;
        contextPtr->result = LE_CLOSED;
    }

    // Stop connection timer.
    if (contextPtr->timerRef != NULL)
    {
        le_timer_Stop(contextPtr->timerRef);
    }

    LE_DEBUG("FTP event received %x state %d", events, contextPtr->controlState);

    do
    {
        restartLoop = false;
        switch (contextPtr->controlState)
        {
            case FTP_CONNECTED:

                status = FtpClientGetResponseCode(contextPtr, &response);
                contextPtr->response = response;
                if ((status == LE_OK) && (response == RESP_SERVER_READY))
                {
                    if (contextPtr->securityMode == LE_FTP_CLIENT_NONSECURE)
                    {
                        msgLen = snprintf(msgBuf, sizeof(msgBuf),
                                          "USER %s\r\n", contextPtr->userStr);
                        LE_FATAL_IF(msgLen >= sizeof(msgBuf), "Failed to build USER command.");
                        status = SendRequestMessage(contextPtr, msgBuf, msgLen);
                        if (LE_OK == status)
                        {
                            contextPtr->controlState = FTP_USER_SENT;
                            restartLoop = true;
                            break;
                        }
                    }
                    else if (contextPtr->securityMode == LE_FTP_CLIENT_SECURE)
                    {
                        msgLen = snprintf(msgBuf, sizeof(msgBuf), "AUTH TLS\r\n");
                        LE_FATAL_IF(msgLen >= sizeof(msgBuf), "Failed to build AUTH command.");
                        status = SendRequestMessage(contextPtr, msgBuf, msgLen);
                        if (LE_OK == status)
                        {
                            contextPtr->controlState = FTP_AUTH_SENT;
                            restartLoop = true;
                            break;
                        }
                    }
                    else
                    {
                        LE_FATAL("Undefined security mode");
                    }
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_AUTH_SENT:

                status = FtpClientGetResponseCode(contextPtr, &response);
                contextPtr->response = response;
                if ((status == LE_OK) && (response == RESP_AUTH_OK))
                {
                    LE_DEBUG("TLS Handshaking...");
                    status = le_socket_SecureConnection(contextPtr->ctrlSocketRef);
                    if (LE_OK == status)
                    {
                        contextPtr->controlState = FTP_TLS_HSHAKE;
                        restartLoop = true;
                        break;
                    }
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_TLS_HSHAKE:

                msgLen = snprintf(msgBuf, sizeof(msgBuf),
                                  "USER %s\r\n", contextPtr->userStr);
                LE_FATAL_IF(msgLen >= sizeof(msgBuf), "Failed to build USER command.");
                status = SendRequestMessage(contextPtr, msgBuf, msgLen);
                if (LE_OK == status)
                {
                    contextPtr->controlState = FTP_USER_SENT;
                    restartLoop = true;
                    break;
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_USER_SENT:

                status = FtpClientGetResponseCode(contextPtr, &response);
                contextPtr->response = response;
                if (status == LE_OK)
                {
                    if (response == RESP_USER_OK)
                    {
                        msgLen = snprintf(msgBuf, sizeof(msgBuf), "PASS %s\r\n",
                                        contextPtr->passwordStr);
                        LE_FATAL_IF(msgLen >= sizeof(msgBuf), "Failed to build PASS command.");
                        status = SendRequestMessage(contextPtr, msgBuf, msgLen);
                        if (LE_OK == status)
                        {
                            contextPtr->controlState = FTP_PASS_SENT;
                            restartLoop = true;
                            break;
                        }
                    }
                    else if (response == RESP_USER_LOGGED_IN)
                    {
                        msgLen = snprintf(msgBuf, sizeof(msgBuf), "PBSZ 0\r\n");
                        LE_FATAL_IF(msgLen >= sizeof(msgBuf), "Failed to build PBSZ command.");
                        status = SendRequestMessage(contextPtr, msgBuf, msgLen);
                        if (LE_OK == status)
                        {
                            contextPtr->controlState = FTP_PBSZ_SENT;
                            restartLoop = true;
                            break;
                        }
                    }
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_PASS_SENT:

                status = FtpClientGetResponseCode(contextPtr, &response);
                contextPtr->response = response;
                if ((status == LE_OK) && (response == RESP_LOGGED_IN))
                {
                    if (contextPtr->securityMode == LE_FTP_CLIENT_SECURE)
                    {
                        msgLen = snprintf(msgBuf, sizeof(msgBuf), "PBSZ 0\r\n");
                        LE_FATAL_IF(msgLen >= sizeof(msgBuf), "Failed to build PBSZ command.");
                        status = SendRequestMessage(contextPtr, msgBuf, msgLen);
                        if (LE_OK == status)
                        {
                            contextPtr->controlState = FTP_PBSZ_SENT;
                            restartLoop = true;
                            break;
                        }
                    }
                    else if (contextPtr->securityMode == LE_FTP_CLIENT_NONSECURE)
                    {
                        contextPtr->result = LE_OK;
                        contextPtr->controlState = FTP_LOGGED;
                        break;
                    }

                    LE_ERROR("Unsupported security mode");
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_PBSZ_SENT:

                status = FtpClientGetResponseCode(contextPtr, &response);
                contextPtr->response = response;
                if ((status == LE_OK) && (response == RESP_COMMAND_OK))
                {
                    msgLen = snprintf(msgBuf, sizeof(msgBuf), "PROT P\r\n");
                    LE_FATAL_IF(msgLen >= sizeof(msgBuf), "Failed to build PROT command.");
                    status = SendRequestMessage(contextPtr, msgBuf, msgLen);
                    if (LE_OK == status)
                    {
                        contextPtr->controlState = FTP_PROT_SENT;
                        restartLoop = true;
                        break;
                    }
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_PROT_SENT:

                status = FtpClientGetResponseCode(contextPtr, &response);
                contextPtr->response = response;
                if ((status == LE_OK) && (response == RESP_COMMAND_OK))
                {
                    contextPtr->result = LE_OK;
                    contextPtr->controlState = FTP_LOGGED;
                    break;
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_TYPE_SENT:

                if (!(events & POLLIN))
                {
                    break;
                }

                status = FtpClientGetResponseCode(contextPtr, &response);
                contextPtr->response = response;
                if ((status == LE_OK) && (response == RESP_COMMAND_OK))
                {
                    if (contextPtr->ipAddrFamily == AF_INET)
                    {
                        msgLen = snprintf(msgBuf, sizeof(msgBuf), "PASV\r\n");
                    }
                    else
                    {
                        msgLen = snprintf(msgBuf, sizeof(msgBuf), "EPSV\r\n");
                    }
                    status = SendRequestMessage(contextPtr, msgBuf, msgLen);
                    if (LE_OK == status)
                    {
                        if (OP_STORE == contextPtr->operation)
                        {
                            restartLoop = true;
                        }
                        contextPtr->controlState = FTP_PASV_SENT;
                        break;
                    }
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_PASV_SENT:

                if (!(events & POLLIN))
                {
                    break;
                }

                status = FtpClientGetServerAddress(contextPtr,
                                                     contextPtr->dsAddrStr,
                                                     LE_CONFIG_FTPCLIENT_SERVER_NAME_MAX,
                                                     &contextPtr->dsPort);

                if (LE_OK == status)
                {
                    switch(contextPtr->targetState)
                    {
                        case FTP_RETR_SENT:
                            msgLen = snprintf(msgBuf, sizeof(msgBuf),
                                              "RETR %s\r\n", contextPtr->remotePath);
                            LE_FATAL_IF(msgLen >= sizeof(msgBuf), "Failed to build RETR command.");
                            break;

                        case FTP_REST_SENT:
                            msgLen = snprintf(msgBuf, sizeof(msgBuf),
                                              "REST %llu\r\n", contextPtr->restOffSet);
                            LE_FATAL_IF(msgLen >= sizeof(msgBuf), "Failed to build REST command.");
                            break;

                        case FTP_APPE_SENT:
                            msgLen = snprintf(msgBuf, sizeof(msgBuf),
                                              "APPE %s\r\n", contextPtr->remotePath);
                            LE_FATAL_IF(msgLen >= sizeof(msgBuf), "Failed to build APPE command.");
                            restartLoop = true;
                            break;

                        case FTP_STOR_SENT:
                            msgLen = snprintf(msgBuf, sizeof(msgBuf),
                                              "STOR %s\r\n", contextPtr->remotePath);
                            LE_FATAL_IF(msgLen >= sizeof(msgBuf), "Failed to build STOR command.");
                            restartLoop = true;
                            break;

                        default:
                            LE_ERROR("Unsupported target state %d", contextPtr->targetState);
                            status = LE_UNSUPPORTED;
                            break;
                    }
                }

                if (LE_OK == status)
                {
                    status = SendRequestMessage(contextPtr, msgBuf, msgLen);
                    if (LE_OK == status)
                    {
                        status = FtpClientConnectDataServer(contextPtr);

                        if (LE_OK == status)
                        {
                            contextPtr->result = LE_OK;
                            contextPtr->controlState = contextPtr->targetState;

                            // Call user defined callback function.
                            if (contextPtr->eventHandlerFunc != NULL)
                            {
                                contextPtr->eventHandlerFunc(contextPtr,
                                                             LE_FTP_CLIENT_EVENT_DATASTART,
                                                             contextPtr->result,
                                                             contextPtr->eventHandlerDataPtr);
                            }

                            break;
                        }
                    }
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_REST_SENT:

                if (!(events & POLLIN))
                {
                    break;
                }

                status = FtpClientGetResponseCode(contextPtr, &response);
                contextPtr->response = response;
                if ((status == LE_OK) && (response == RESP_REST_AT))
                {
                    msgLen = snprintf(msgBuf, sizeof(msgBuf),
                                      "RETR %s\r\n", contextPtr->remotePath);
                    LE_FATAL_IF(msgLen >= sizeof(msgBuf), "Failed to build RETR command.");
                    status = SendRequestMessage(contextPtr, msgBuf, msgLen);
                    if (LE_OK == status)
                    {
                        contextPtr->controlState = FTP_RETR_SENT;
                        break;
                    }
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_RETR_SENT:

                if (!(events & POLLIN))
                {
                    break;
                }

                // It's possible that both 150 and 226 are received in one le_socket_Read() call,
                // thus here we need try to read response code of second response message.
                status = FtpClientGetMultiResponseCode(contextPtr, &response, &response1);
                contextPtr->response = response;
                if (status == LE_OK)
                {
                    if ((response == RESP_DC_OPENED) || (response == RESP_OPENING_DC))
                    {
                        if (response1 == RESP_TRANS_OK)
                        {
                            LE_INFO("XFEREND notification received.");
                            // Save the expected second response code.
                            contextPtr->response = response1;
                            if(contextPtr->recvDone)
                            {
                                // Data transfer is done, we can safely close the data session.
                                FtpClientDisconnectDataServer(contextPtr);
                                contextPtr->controlState = FTP_DATAEND;
                                restartLoop = true;
                            }
                            else
                            {
                                contextPtr->controlState = FTP_XFEREND;
                            }
                            break;
                        }
                        else if(response1 == RESP_INVALID)
                        {
                            // No second response code , data transfer is ongoing.
                            contextPtr->controlState = FTP_XFERING;
                            break;
                        }
                        else
                        {
                            // Save the unexpected second response code if exists.
                            LE_ERROR("Unexpected second response code %d.", response1);
                            contextPtr->response = response1;
                        }
                    }
                    else if (response == RESP_NO_FILE)
                    {
                        // Filename is unavailable, retain the connection on control session
                        // and close the data session.
                        FtpClientDisconnectDataServer(contextPtr);
                        contextPtr->controlState = FTP_DATAEND;
                        restartLoop = true;
                        break;
                    }
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_APPE_SENT:
            case FTP_STOR_SENT:

                if (!(events & POLLIN))
                {
                    break;
                }

                status = FtpClientGetResponseCode(contextPtr, &response);
                contextPtr->response = response;
                if (status == LE_OK)
                {
                    if ((response == RESP_DC_OPENED) || (response == RESP_OPENING_DC))
                    {
                        // Get back into async mode
                        if (LE_OK == le_socket_SetMonitoring(contextPtr->ctrlSocketRef, true))
                        {
                            contextPtr->controlState = FTP_XFERING;
                            break;
                        }
                    }
                    else if (response == RESP_NO_FILE)
                    {
                        // Filename is unavailable, retain the connection on control session
                        // and close the data session.
                        FtpClientDisconnectDataServer(contextPtr);
                        contextPtr->controlState = FTP_DATAEND;
                        restartLoop = true;
                        break;
                    }
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_SIZE_SENT:

                status = FtpClientGetFileSize(contextPtr, &contextPtr->fileSize);
                if (status == LE_OK || status == LE_NOT_FOUND)
                {
                    contextPtr->controlState = FTP_LOGGED;
                    contextPtr->result = status;
                    break;
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_DELE_SENT:

                status = FtpClientGetResponseCode(contextPtr, &response);
                contextPtr->response = response;

                if (LE_OK == status)
                {
                    if (response == RESP_DELE_OK || response == RESP_NO_FILE)
                    {
                        contextPtr->controlState = FTP_LOGGED;
                        contextPtr->result = (response == RESP_NO_FILE) ? LE_NOT_FOUND : LE_OK;
                        break;
                    }
                }

                // Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;


            case FTP_XFERING:

                if (!(events & POLLIN))
                {
                    break;
                }

                status = FtpClientGetResponseCode(contextPtr, &response);
                contextPtr->response = response;
                if (status == LE_OK)
                {
                    if (response == RESP_TRANS_OK)
                    {
                        LE_INFO("XFEREND notification received.");
                        if(contextPtr->recvDone)
                        {
                            // Data transfer is done, we can safely close the data session.
                            FtpClientDisconnectDataServer(contextPtr);
                            contextPtr->controlState = FTP_DATAEND;
                            restartLoop = true;
                        }
                        else
                        {
                            // Data transfer is not done , defer to close the data session.
                            contextPtr->controlState = FTP_XFEREND;
                        }
                        break;
                    }
                }

                //Fail case.
                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_DATAEND:

                contextPtr->controlState = FTP_LOGGED;
                contextPtr->operation = OP_NONE;
                contextPtr->result = LE_OK;

                // Data transfering is completed, call user defined callback function.
                if (contextPtr->eventHandlerFunc)
                {
                    if (contextPtr->response == RESP_TRANS_OK)
                    {
                        // Data transfering successfully.
                        callbackEvent = LE_FTP_CLIENT_EVENT_DATAEND;
                    }
                    else
                    {
                        // Operation on unavailable file, will AT ERROR response.
                        callbackEvent = LE_FTP_CLIENT_EVENT_ERROR;
                    }
                    contextPtr->eventHandlerFunc(contextPtr,
                                                 callbackEvent,
                                                 contextPtr->result,
                                                 contextPtr->eventHandlerDataPtr);
                }
                break;

            case FTP_QUIT:

                msgLen = snprintf(msgBuf, sizeof(msgBuf), "QUIT\r\n");
                status = SendRequestMessage(contextPtr, msgBuf, msgLen);
                if (status == LE_OK)
                {
                    contextPtr->controlState = FTP_QUIT_SENT;
                }
                else
                {
                    contextPtr->controlState = FTP_CLOSING;
                }

                contextPtr->result = status;
                restartLoop = true;
                break;

            case FTP_QUIT_SENT:

                status = FtpClientGetResponseCode(contextPtr, &response);
                contextPtr->response = response;
                if ((status == LE_OK) && (response == RESP_QUIT_OK))
                {
                    LE_INFO("QUIT OK.");
                }
                else
                {
                    LE_ERROR("QUIT failed");
                }

                contextPtr->result = status;
                contextPtr->controlState = FTP_CLOSING;
                restartLoop = true;
                break;

            case FTP_CLOSING:

                LE_INFO("Closing session (op = %d, response = %d, result = %d).",
                        contextPtr->operation, contextPtr->response, contextPtr->result);
                FtpClientClose(contextPtr);
                sessionClosed = true;

                if (IsBlocking(contextPtr->operation))
                {
                    break;
                }

                // For non-blocking operations call user defined callback function.
                if (OP_NONE == contextPtr->operation)
                {
                    // Already logged into server but no command is running.
                    callbackEvent = LE_FTP_CLIENT_EVENT_CLOSED;
                }
                else
                {
                    // STOR or RETR command is running, will AT ERROR response.
                    callbackEvent = LE_FTP_CLIENT_EVENT_ERROR;
                }

                contextPtr->operation = OP_NONE;
                if (contextPtr->eventHandlerFunc != NULL)
                {
                    contextPtr->eventHandlerFunc(contextPtr,
                                                 callbackEvent,
                                                 contextPtr->result,
                                                 contextPtr->eventHandlerDataPtr);
                }
                break;

            default:

                LE_INFO("Checking session in state (%d)", contextPtr->controlState);
                if (events & POLLIN)
                {
                    // Check if session is closed by remote server.
                    if (LE_OK != FtpClientGetResponseCode(contextPtr, &response))
                    {
                        contextPtr->controlState = FTP_CLOSING;
                        contextPtr->result = LE_CLOSED;
                        restartLoop = true;
                    }
                    else
                    {
                        contextPtr->response = response;
                    }
                }
                break;
        }
    }
    while (restartLoop);

    if (!sessionClosed && (contextPtr->timerRef != NULL))
    {
        le_timer_Start(contextPtr->timerRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Do name resolution with IPv4 or IPv6
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FtpServerDnsQuery
(
    le_ftpClient_SessionRef_t sessionRef, ///< [IN] ftp client session ref
    char *buf,                            ///< [OUT] Buffer for storing IP address
    size_t bufLen                         ///< [IN] Buffer size in bytes
)
{
    struct addrinfo hints, *servInfo;
    char serverIp[INET6_ADDRSTRLEN]={0};

    memset(&hints, 0, sizeof(hints));

    sessionRef->ipAddrFamily = AF_UNSPEC;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

    if (getaddrinfo(sessionRef->serverStr, NULL, &hints, &servInfo) != 0)
    {
        hints.ai_family = AF_INET6;
        if (getaddrinfo(sessionRef->serverStr, NULL, &hints, &servInfo) != 0)
        {
            LE_ERROR("Error on function: getaddrinfo");
            return LE_UNAVAILABLE;
        }
        sessionRef->ipAddrFamily = AF_INET6;
    }
    else
    {
        sessionRef->ipAddrFamily = AF_INET;
    }

    if (AF_INET == sessionRef->ipAddrFamily)
    {
        if (NULL == inet_ntop(AF_INET, &(((struct sockaddr_in *)servInfo->ai_addr)->sin_addr),
                serverIp, INET6_ADDRSTRLEN))
        {
            LE_ERROR("Error on inet_ntop(AF_INET).");
            freeaddrinfo(servInfo);
            return LE_FAULT;
        }
    }
    else
    {
        if (NULL == inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)servInfo->ai_addr)->sin6_addr),
                serverIp, INET6_ADDRSTRLEN))
        {
            LE_ERROR("Error on inet_ntop(AF_INET6).");
            freeaddrinfo(servInfo);
            return LE_FAULT;
        }
    }

    freeaddrinfo(servInfo);
    strlcpy(buf, serverIp, bufLen);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Connect to a remote FTP server with specified usr/pwd
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FtpClientConnectServer
(
    le_ftpClient_SessionRef_t sessionRef  ///< [IN] ftp client session ref
)
{
    LE_FATAL_IF(sessionRef == NULL, "FTP client session is NULL.");

    if (sessionRef->isConnected)
    {
        return LE_OK;
    }

    LE_FATAL_IF(sessionRef->ctrlSocketRef != NULL, "control socket is already created!");
    LE_FATAL_IF(sessionRef->timerRef != NULL, "connection timer is already created!");
    LE_FATAL_IF((sessionRef->securityMode == LE_FTP_CLIENT_SECURE)
                    && (sessionRef->certPtr == NULL), "Null certificate passed");

    // Create the connection timer.
    if ( LE_OK != FtpClientCreateTimer(sessionRef))
    {
        return LE_FAULT;
    }

    char serverIpAddr[LE_MDC_IPV6_ADDR_MAX_BYTES] = {0};
    if (FtpServerDnsQuery(sessionRef, serverIpAddr, sizeof(serverIpAddr)) != LE_OK)
    {
        LE_ERROR("Failed to query the IP address of server %s", sessionRef->serverStr);
        return LE_UNAVAILABLE;
    }

    // Create the control socket.
    sessionRef->ctrlSocketRef = le_socket_Create(serverIpAddr,
                                                 sessionRef->serverPort,
                                                 sessionRef->srcIpAddr, TCP_TYPE);
    if (NULL == sessionRef->ctrlSocketRef)
    {
        LE_ERROR("Failed to create control socket for server %s:%u.", sessionRef->serverStr,
                 sessionRef->serverPort);
        return LE_FAULT;
    }

    // Set socket timeout (1s).
    if (LE_OK != le_socket_SetTimeout(sessionRef->ctrlSocketRef, FTP_TIMEOUT_MS))
    {
       LE_ERROR("Failed to set response timeout.");
       goto freeSocket;
    }

    if (sessionRef->securityMode == LE_FTP_CLIENT_SECURE)
    {
        if (le_socket_SetCipherSuites(sessionRef->ctrlSocketRef, sessionRef->cipherIdx) != LE_OK)
        {
            LE_ERROR("Failed to set cipher suites.");
            goto freeSocket;
        }
    }

    // Connect the remote FTP server.
    if (LE_OK != le_socket_Connect(sessionRef->ctrlSocketRef))
    {
        LE_ERROR("Failed to connect FTP server %s:%u.", sessionRef->serverStr,
                 sessionRef->serverPort);
        goto freeSocket;
    }

    if (sessionRef->securityMode == LE_FTP_CLIENT_SECURE)
    {
        if (le_socket_AddCertificate(sessionRef->ctrlSocketRef,
                                     sessionRef->certPtr,
                                     sessionRef->certSize) != LE_OK)
        {
            LE_ERROR("Failed to add root CA certificates.");
            goto freeSocket;
        }
    }

    // For connect command call FtpClientStateMachine() in sync mode.
    sessionRef->controlState = FTP_CONNECTED;
    FtpClientStateMachine(sessionRef->ctrlSocketRef, POLLOUT, sessionRef);

    // Check the result.
    if ((sessionRef->controlState == FTP_LOGGED) &&
        (sessionRef->result == LE_OK) &&
            ((sessionRef->response == RESP_LOGGED_IN) ||
             (sessionRef->response == RESP_COMMAND_OK)) )
    {
        // Set the socket event callback for fd monitor.
        if (LE_OK != le_socket_AddEventHandler(sessionRef->ctrlSocketRef,
                                               FtpClientStateMachine,sessionRef))
        {
            LE_ERROR("Failed to add socket event handler.");
            goto freeSocket;
        }

        // Enable async mode and start fd monitor to monitor the disconnect event.
        if (LE_OK != le_socket_SetMonitoring(sessionRef->ctrlSocketRef, true))
        {
            LE_ERROR("Failed to enable socket monitor.");
            goto freeSocket;
        }

        // Succeed to login server with specified usr/pwd.
        LE_INFO("Succeed to login FTP server %s:%u in session (%p).", sessionRef->serverStr,
                sessionRef->serverPort, sessionRef);

        sessionRef->isConnected = true;

        return LE_OK;
    }
    else
    {
        LE_ERROR("Failed to login FTP server %s:%u with %s/%s.", sessionRef->serverStr,
                 sessionRef->serverPort, sessionRef->userStr, sessionRef->passwordStr);
    }

freeSocket:
    if (sessionRef->ctrlSocketRef != NULL)
    {
        le_socket_Delete(sessionRef->ctrlSocketRef);
        sessionRef->ctrlSocketRef = NULL;
    }
    if (sessionRef->timerRef != NULL)
    {
        le_timer_Delete(sessionRef->timerRef);
        sessionRef->timerRef = NULL;
    }
    sessionRef->controlState = FTP_CLOSED;
    sessionRef->isConnected = false;
    return LE_COMM_ERROR;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Disconnect to the remote FTP server
 */
//--------------------------------------------------------------------------------------------------
static void FtpClientDisconnectServer
(
    le_ftpClient_SessionRef_t sessionRef  ///< [IN] ftp client session ref
)
{
    LE_FATAL_IF(sessionRef == NULL, "FTP client session is NULL.");

    if (!sessionRef->isConnected)
    {
        return;
    }

    if ((sessionRef->controlState == FTP_LOGGED) &&
        (sessionRef->operation == OP_DISCONNECT))
    {
        // For disconnect command call FtpClientStateMachine() in sync mode.
        sessionRef->controlState = FTP_QUIT;
        FtpClientStateMachine(sessionRef->ctrlSocketRef, POLLOUT, sessionRef);
    }
    else
    {
        // For other cases force to close the client.
        FtpClientClose(sessionRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Create a new FTP client session.
 *
 *  @warning    For plain FTP, transmission of the credentials and files will be done without
 *              encryption.
 *
 *  @return
 *      - A new FTP session reference on success.
 *      - NULL if an error occured.
 */
//--------------------------------------------------------------------------------------------------
le_ftpClient_SessionRef_t le_ftpClient_CreateSession
(
    const char      *serverStr,     ///< Host name or IP address of target FTP server.
    uint16_t         port,          ///< FTP server control port.  Use
                                    ///  LE_FTP_CLIENT_DEFAULT_CTRL_PORT for the default.
    const char      *userStr,       ///< User name to log into the server under.
    const char      *passwordStr,   ///< Password to use when logging in to the server.
    unsigned int     timeout        ///< Connection timeout in seconds.
)
{
    le_ftpClient_SessionRef_t sessionRef;

    // Validate the parameters
    if (serverStr == NULL       ||
        serverStr[0] == '\0'    ||
        strlen(serverStr) >= LE_CONFIG_FTPCLIENT_SERVER_NAME_MAX)
    {
        LE_FATAL("Invalid serverStr.");
    }
    if (userStr == NULL ||
        strlen(userStr) >= LE_CONFIG_FTPCLIENT_USER_NAME_MAX)
    {
        LE_FATAL("Invalid userStr.");
    }
    if (passwordStr == NULL ||
        strlen(passwordStr) >= LE_CONFIG_FTPCLIENT_PASSWORD_MAX)
    {
        LE_FATAL("Invalid passwordStr.");
    }

    // Allocate the session
    sessionRef = le_mem_TryAlloc(SessionPool);
    if (sessionRef == NULL)
    {
        LE_ERROR("No more sessions available.");
        return NULL;
    }
    memset(sessionRef, 0, sizeof(struct le_ftpClient_Session));

    // Populate the session
    strlcpy(sessionRef->serverStr, serverStr, sizeof(sessionRef->serverStr));
    strlcpy(sessionRef->userStr, userStr, sizeof(sessionRef->userStr));
    strlcpy(sessionRef->passwordStr, passwordStr, sizeof(sessionRef->passwordStr));
    sessionRef->operation = OP_NONE;
    sessionRef->serverPort= port;
    sessionRef->timeout = timeout * 1000;
    sessionRef->ipAddrFamily = AF_UNSPEC;
    sessionRef->isConnected = false;
    sessionRef->controlState = FTP_CLOSED;
    sessionRef->targetState = FTP_CLOSED;
    sessionRef->result = LE_OK;
    sessionRef->ctrlSocketRef = NULL;
    sessionRef->dataSocketRef = NULL;
    sessionRef->timerRef = NULL;
    sessionRef->certPtr = NULL;
    sessionRef->certSize = 0;

    LE_INFO("Created FTP client session (%p).", sessionRef);
    return sessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Close and destroy an FTP client session.
 */
//--------------------------------------------------------------------------------------------------
void le_ftpClient_DestroySession
(
    le_ftpClient_SessionRef_t sessionRef    ///< Session reference.
)
{
    if (sessionRef == NULL)
    {
        return;
    }

    le_ftpClient_SetEventCallback(sessionRef, NULL, NULL);
    le_ftpClient_Disconnect(sessionRef);

    le_mem_Release(sessionRef);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Set a callback to be invoked to handle asynchronous session events.
 *  The possible event types are described by the le_ftpClient_Event_t enum.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_ftpClient_SetEventCallback
(
    le_ftpClient_SessionRef_t    sessionRef,    ///< Session reference.
    le_ftpClient_EventFunc_t     handlerFunc,   ///< Handler callback.  May be NULL to remove the
                                                ///  handler.
    void                        *userPtr        ///< Additional data to pass to the handler.
)
{
    if (sessionRef == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    sessionRef->eventHandlerFunc = handlerFunc;
    sessionRef->eventHandlerDataPtr = (handlerFunc == NULL ? NULL : userPtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set cipher suites used in establishing secure connection
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ftpClient_SetCipherSuites
(
    le_ftpClient_SessionRef_t    sessionRef,    ///< Session reference.
    uint8_t                      cipherIdx      ///< Cipher suites index
)
{
    if (sessionRef == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    sessionRef->cipherIdx = cipherIdx;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Open a new connection on dedicate source address to the configured server.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ftpClient_ConnectOnSrcAddr
(
    le_ftpClient_SessionRef_t sessionRef,   ///< Session reference.
    char*                     srcAddr       ///< [IN] Source Address of PDP profile.
)
{
    le_result_t  result;

    if (sessionRef == NULL || srcAddr == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    if (sessionRef->isConnected)
    {
        return LE_OK;
    }

    memcpy(sessionRef->srcIpAddr, srcAddr, LE_MDC_IPV6_ADDR_MAX_BYTES);
    // Start sync operation.
    sessionRef->operation = OP_CONNECT;
    sessionRef->securityMode = LE_FTP_CLIENT_NONSECURE;
    result = FtpClientConnectServer(sessionRef);
    sessionRef->isConnected = (result == LE_OK);
    sessionRef->operation = OP_NONE;
    // End sync operation.

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Open a new secure connection on a dedicated source address to the configured server.
 *
 *  @note certificatePtr must be allocated via le_mem API
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_ftpClient_SecureConnectOnSrcAddr
(
    le_ftpClient_SessionRef_t sessionRef,       ///< [IN] Session reference.
    char*                     srcAddr,          ///< [IN] Source Address of PDP profile.
    const uint8_t*            certificatePtr,   ///< [IN] Pointer to certificate. Data buffer must
                                                ///<      be allocated via le_mem API
    size_t                    certificateLen    ///< [IN] Certificate Length
)
{
    le_result_t  result;

    if ((sessionRef == NULL) || (certificatePtr == NULL) || (srcAddr == NULL))
    {
        return LE_BAD_PARAMETER;
    }

    if (sessionRef->isConnected)
    {
        return LE_OK;
    }

    // Store reference to certificate
    le_mem_AddRef((void *)certificatePtr);
    sessionRef->certPtr = certificatePtr;
    sessionRef->certSize = certificateLen;

    // Set session source address
    memcpy(sessionRef->srcIpAddr, srcAddr, LE_MDC_IPV6_ADDR_MAX_BYTES);

    // Start sync operation.
    sessionRef->operation = OP_CONNECT;
    sessionRef->securityMode = LE_FTP_CLIENT_SECURE;
    result = FtpClientConnectServer(sessionRef);
    sessionRef->isConnected = (result == LE_OK);
    sessionRef->operation = OP_NONE;
    // End sync operation.

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Open a new connection to the configured server.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ftpClient_Connect
(
    le_ftpClient_SessionRef_t sessionRef    ///< Session reference.
)
{
    char srcIpAddress[LE_MDC_IPV6_ADDR_MAX_BYTES] = {0};

    // use default profile
    le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile((uint32_t)LE_MDC_DEFAULT_PROFILE);
    if(!profileRef)
    {
        LE_ERROR("le_mdc_GetProfile cannot get default profile");
        return LE_FAULT;
    }
    // Try IPv4, then Ipv6
    if (LE_OK == le_mdc_GetIPv4Address(profileRef, srcIpAddress, sizeof(srcIpAddress)))
    {
        LE_INFO("le_ftpClient_Connect using IPv4 profile & source addr %s", srcIpAddress);
    }
    else if (LE_OK == le_mdc_GetIPv6Address(profileRef, srcIpAddress, sizeof(srcIpAddress)))
    {
        LE_INFO("le_ftpClient_Connect using IPv6 profile & source addr %s", srcIpAddress);
    }
    else
    {
        LE_ERROR("le_ftpClient_Connect No IPv4 or IPv6 profile");
        return LE_FAULT;
    }

    return le_ftpClient_ConnectOnSrcAddr(sessionRef, srcIpAddress);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Close the active connection.  A new connection may be opened with le_ftpClient_Connect().
 */
//--------------------------------------------------------------------------------------------------
void le_ftpClient_Disconnect
(
    le_ftpClient_SessionRef_t sessionRef    ///< Session reference.
)
{
    if (sessionRef == NULL)
    {
        return;
    }

    if (sessionRef->isConnected)
    {
        // Start sync operation.
        sessionRef->operation = OP_DISCONNECT;
        FtpClientDisconnectServer(sessionRef);
        sessionRef->isConnected = false;
        sessionRef->operation = OP_NONE;
        // End sync operation.
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Get information about the FTP session.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_ftpClient_GetInfo
(
    le_ftpClient_SessionRef_t    sessionRef,    ///< [IN]  Session reference.
    le_ftpClient_Info_t         *infoPtr        ///< [OUT] Session information.

)
{
    if (sessionRef == NULL || infoPtr == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    infoPtr->serverStr = sessionRef->serverStr;
    infoPtr->port = sessionRef->serverPort;
    infoPtr->addressFamily = sessionRef->ipAddrFamily;
    infoPtr->userStr = sessionRef->userStr;
    infoPtr->mode = LE_FTP_CLIENT_CONNECTION_PASSIVE;
    infoPtr->isConnected = sessionRef->isConnected;
    infoPtr->isRunning = (sessionRef->operation != OP_NONE);
    infoPtr->response = sessionRef->response;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Retrieve a file from the remote server.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ftpClient_Retrieve
(
    le_ftpClient_SessionRef_t    sessionRef,    ///< Session reference.
    const char                  *pathStr,       ///< File path on remote server.
    le_ftpClient_TransferType_t  type,          ///< Transfer type.
    uint64_t                     offset,        ///< Offset in the remote file to begin downloading
                                                ///  from.  This allows partial transfers to be
                                                ///  resumed.
    le_ftpClient_WriteFunc_t     writeFunc,     ///< Callback to invoke to handle writing out
                                                ///  downloaded file.
    void                        *writePtr       ///< Arbitrary user data to be passed to the write
                                                ///  callback.  May be NULL.
)
{
    le_result_t status;
    char msgBuf[FTP_RESP_MAX_SIZE];
    int msgLen;

    if (sessionRef == NULL || pathStr == NULL || writeFunc == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    if (type != LE_FTP_CLIENT_TRANSFER_BINARY)
    {
        return LE_NOT_IMPLEMENTED;
    }

    if (!sessionRef->isConnected ||
        sessionRef->controlState != FTP_LOGGED ||
        sessionRef->ctrlSocketRef == NULL)
    {
        return LE_NOT_PERMITTED;
    }

    msgLen = snprintf(msgBuf, sizeof(msgBuf), "TYPE I\r\n");
    status = SendRequestMessage(sessionRef, msgBuf, msgLen);
    if (status == LE_OK)
    {
        // Start async operation. Then end operation is done in callback function.
        sessionRef->operation = OP_RETRIEVE;
        sessionRef->remotePath = pathStr;
        sessionRef->restOffSet = offset;
        sessionRef->writeFunc = writeFunc;
        sessionRef->userDataPtr = writePtr;
        sessionRef->controlState = FTP_TYPE_SENT;
        sessionRef->targetState = sessionRef->restOffSet > 0 ? FTP_REST_SENT : FTP_RETR_SENT;
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Upload a file to the remote server.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ftpClient_Store
(
    le_ftpClient_SessionRef_t    sessionRef,    ///< Session reference.
    const char                  *pathStr,       ///< File path on remote server.
    le_ftpClient_TransferType_t  type,          ///< Transfer type.
    bool                         append         ///< Append to the remote file, rather than
                                                ///  overwriting it.
)
{
    le_result_t status;
    char msgBuf[FTP_RESP_MAX_SIZE];
    int msgLen;

    if (sessionRef == NULL || pathStr == NULL)
    {
        return LE_BAD_PARAMETER;
    }
    if (type != LE_FTP_CLIENT_TRANSFER_BINARY)
    {
        return LE_NOT_IMPLEMENTED;
    }

    if (!sessionRef->isConnected ||
        sessionRef->controlState != FTP_LOGGED ||
        sessionRef->ctrlSocketRef == NULL)
    {
        return LE_NOT_PERMITTED;
    }

    // Start async operation. Then end operation is done in callback function.
    sessionRef->remotePath = pathStr;
    msgLen = snprintf(msgBuf, sizeof(msgBuf), "TYPE I\r\n");
    status = SendRequestMessage(sessionRef, msgBuf, msgLen);
    if (status == LE_OK)
    {
        sessionRef->operation = OP_STORE;
        sessionRef->controlState = FTP_TYPE_SENT;
        if (append)
        {
            sessionRef->targetState = FTP_APPE_SENT;
        }
        else
        {
            sessionRef->targetState = FTP_STOR_SENT;
        }

        // Disable monitoring and switch to sync mode until ready to send
        le_socket_SetMonitoring(sessionRef->ctrlSocketRef, false);
        FtpClientStateMachine(sessionRef->ctrlSocketRef, POLLIN, sessionRef);
        status = sessionRef->result;
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send some file data to the remote server.  A store operation must be active when this function
 *  is called.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ftpClient_Send
(
    le_ftpClient_SessionRef_t    sessionRef,    ///< [IN]     Session reference.
    const void                  *dataPtr,       ///< [IN]     Data to send.
    size_t                      *length,        ///< [IN,OUT] Length of data to send as input.  The
                                                ///<          number of bytes remaining to be sent
                                                ///<          as output.  If this is greater than 0
                                                ///<          (LE_NO_MEMORY is returned) then
                                                ///<          le_ftpClient_Send() must be called
                                                ///<          again later with the remainder of the
                                                ///<          buffer.
    bool                         done           ///< [IN]     Indicates this is the last piece of
                                                ///<          data for the store operation.  Setting
                                                ///<          this to true will end the store after
                                                ///<          the last data is sent.
)
{
    size_t bufLen = 0;
    le_result_t result;

    if (sessionRef == NULL || sessionRef->operation != OP_STORE)
    {
        return LE_BAD_PARAMETER;
    }

    if (*length > 0)
    {
        bufLen = MIN(LE_CONFIG_FTPCLIENT_BUFFER_SIZE, *length);
        result = le_socket_Send(sessionRef->dataSocketRef, (char*)dataPtr, bufLen);
        if (result != LE_OK)
        {
            return result;
        }
        *length -= bufLen;
    }

    if (*length == 0 && done)
    {
        sessionRef->recvDone = true;
        FtpClientDisconnectDataServer(sessionRef);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Delete a file from the remote server.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ftpClient_Delete
(
    le_ftpClient_SessionRef_t    sessionRef,    ///< Session reference.
    const char                  *pathStr        ///< File path on remote server.
)
{
    char msgBuf[FTP_RESP_MAX_SIZE] = {0};
    int msgLen;

    if (sessionRef == NULL || pathStr == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    if (!sessionRef->isConnected ||
        sessionRef->controlState != FTP_LOGGED ||
        sessionRef->ctrlSocketRef == NULL)
    {
        return LE_NOT_PERMITTED;
    }

    //main handle function
    sessionRef->remotePath = pathStr;
    msgLen = snprintf(msgBuf, sizeof(msgBuf), "DELE %s\r\n", sessionRef->remotePath);
    if (LE_OK == SendRequestMessage(sessionRef, msgBuf, msgLen))
    {
        sessionRef->operation = OP_DELETE;
        sessionRef->controlState = FTP_DELE_SENT;
        FtpClientStateMachine(sessionRef->ctrlSocketRef, POLLOUT, sessionRef);
        sessionRef->operation = OP_NONE;
        return sessionRef->result;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Query the size of a file on the remote server.  This can be used for determining the appropriate
 *  data to upload when resuming a Store command.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ftpClient_Size
(
    le_ftpClient_SessionRef_t    sessionRef,    ///< [IN]  Session reference.
    const char                  *pathStr,       ///< [IN]  File path on remote server.
    le_ftpClient_TransferType_t  type,          ///< [IN]  Transfer type.
    uint64_t                    *sizePtr        ///< [OUT] Size of remote file in bytes.
)
{
    char msgBuf[FTP_RESP_MAX_SIZE] = {0};
    int msgLen;

    if (sessionRef == NULL || pathStr == NULL || sizePtr == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    if (type != LE_FTP_CLIENT_TRANSFER_BINARY)
    {
        return LE_NOT_IMPLEMENTED;
    }

    if (!sessionRef->isConnected ||
        sessionRef->controlState != FTP_LOGGED ||
        sessionRef->ctrlSocketRef == NULL)
    {
        return LE_NOT_PERMITTED;
    }

    sessionRef->remotePath = pathStr;
    msgLen = snprintf(msgBuf, sizeof(msgBuf), "SIZE %s\r\n", sessionRef->remotePath);
    if (LE_OK == SendRequestMessage(sessionRef, msgBuf, msgLen))
    {
        sessionRef->operation = OP_SIZE;
        sessionRef->controlState = FTP_SIZE_SENT;
        FtpClientStateMachine(sessionRef->ctrlSocketRef, POLLOUT, sessionRef);
        sessionRef->operation = OP_NONE;
        *sizePtr = sessionRef->fileSize;
        return sessionRef->result;
    }

    return LE_FAULT;
}
