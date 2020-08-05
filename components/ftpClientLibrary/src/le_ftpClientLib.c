/**
 * @file le_ftpClientLib.c
 *
 * Implementation of Legato FTP client built on top of le_socketLib.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "../le_ftpClient.h"
#include "legato.h"
#include "interfaces.h"

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
 *  FTP client session.  This is a wrapper around a lwftp session.
 */
//--------------------------------------------------------------------------------------------------
struct le_ftpClient_Session
{
    char    serverStr[LE_CONFIG_FTPCLIENT_SERVER_NAME_MAX]; ///< Server hostname/address.
    char    userStr[LE_CONFIG_FTPCLIENT_USER_NAME_MAX];     ///< User name.
    char    passwordStr[LE_CONFIG_FTPCLIENT_PASSWORD_MAX];  ///< User's password.

    enum Operation               operation;     ///< Current operation.
    union
    {
        le_ftpClient_WriteFunc_t    writeFunc;  ///< Callback to write downloaded data.
        uint64_t                    fileSize;   ///< Size of remote file.
    };

    // For blocking operations:
    bool                         isConnected;   ///< Connection status.
    int                          result;        ///< Result of the current blocking operation.
    uint8_t                      eventIdIndex;  ///< Index of the event Id for current session
    le_sem_Ref_t                 semRef;        ///< Semaphore for signalling completion of blocking
                                                ///< calls.
    void                        *userDataPtr;   ///< Arbitrary user supplied data.

    // For asynchronous operations:
    bool                         needsResume;           ///< Trigger lwftp resume on next send.for asynchronous operation
    le_event_HandlerRef_t        eventHandlerRef;       ///< Event handler for asynchronous events.
    le_ftpClient_EventFunc_t     eventHandlerFunc;      ///< Asynchronous event handler callback.

    le_mutex_Ref_t               mutexRef;              ///< Event queue mutex.
    le_sls_List_t                eventQueue;            ///< Event queue.
    void                        *eventHandlerDataPtr;   ///< User data to pass to event handler.

    //lwftp_session_t              lwftp;         ///< lwftp session instance.
};

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
  return;
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
    return NULL;
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
    return;
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
    return LE_NOT_IMPLEMENTED;
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
    return LE_NOT_IMPLEMENTED;
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
    return;
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
    return LE_NOT_IMPLEMENTED;
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
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send some file data to the remote server.  A store operation must be active when this function
 *  is called.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
//vchen, need some way to infrom the monitoring thread to send data.
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
    return LE_NOT_IMPLEMENTED;
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
    return LE_NOT_IMPLEMENTED;
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
    return LE_NOT_IMPLEMENTED;
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
    return LE_NOT_IMPLEMENTED;
}
