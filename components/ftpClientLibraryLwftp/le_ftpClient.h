/**
 * @page c_le_ftpClient FTP Client
 *
 * @ref le_ftpClient.h "API Reference"
 *
 * <HR>
 *
 * @section ftp_overview Overview
 *
 * The FTP client library provides applications with the ability to make FTP requests to remote
 * servers for the purposes of uploading or downloading files.
 *
 * @note   The FTP client library only supports binary passive mode FTP transfers.  Active transfer
 *         mode and the ASCII data transfer format are not supported.
 *
 * This FTP client requires that the remote server support the following
 * [RFC959](https://tools.ietf.org/html/rfc959), [RFC2428](https://tools.ietf.org/html/rfc2428),
 * and [RFC3659](https://tools.ietf.org/html/rfc3659) compliant commands:
 *     - `APPE`
 *     - `DELE`
 *     - `EPSV` (only if IPv6 is used)
 *     - `PASS`
 *     - `PASV`
 *     - `QUIT`
 *     - `REST`
 *     - `RETR`
 *     - `SIZE`
 *     - `STOR`
 *     - `TYPE`
 *     - `USER`
 *
 * The operations provided by this API enable:
 *     - Requesting the size of a file on the server.
 *     - Uploading a file to the server, including resuming a partial upload/appending to a file.
 *     - Downloading a file from the server, including resuming a partial download.
 *     - Deleting a file from the server.
 *
 * The number of simultaneous FTP sessions permitted is controlled by the FTPCLIENT_SESSION_MAX
 * KConfig setting, and defaults to 2.  Attempting to open more sessions than the maximum will
 * return NULL session handles.
 *
 * @section ftp_client_include Interface Requirements
 *
 * To use the FTP client, add the following to the Component.cdef of the component that requires
 * the library:
 *
 * @code
requires:
{
    component:
    {
        $LEGATO_ROOT/components/ftpClientLibrary
    }
}

cflags:
{
    -I$LEGATO_ROOT/components/ftpClientLibrary
}
 * @endcode
 *
 * Then, in the component's code, just `#include "le_ftpClient.h"`.
 *
 * @section ftp_client_behaviour Behaviour
 *
 * Before any file transfer operations can be performed, a session must be opened by supplying the
 * necessary connection information and credentials to le_ftpClient_CreateSession().  Once a
 * session has been obtained, the server connection may be established by calling
 * le_ftpClient_Connect(), and closed with le_ftpClient_Disconnect().  The connection for a session
 * may be opened and closed multiple times by the user, though only one connection may be active at
 * a time.  If it is no longer needed, the session should be destroyed with
 * le_ftpClient_DestroySession(), which will also close the active connection, if any.
 *
 * Connected sessions can generate asynchronous events when certain conditions occur.  The supported
 * events are enumerated in le_ftpClient_Event_t.  The user may choose to register for these
 * callbacks using the le_ftpClient_SetEventCallback() function.  The event callback will be
 * serviced in the Legato event loop of the current thread.
 *
 * The main feature functions of the library are:
 *     - le_ftpClient_Retrieve()
 *     - le_ftpClient_Store()
 *     - le_ftpClient_Send()
 *     - le_ftpClient_Delete()
 *     - le_ftpClient_Size()
 *
 * The size and delete functions operate synchronously with respect to the calling thread.  The
 * retrieve and store functions operate asynchronously, and their results are delivered via the
 * event callback.
 *
 * For a store operation, the action is started by calling le_ftpClient_Store().  Then
 * le_ftpClient_Send() must be called as many times as necessary to provide the file content for
 * upload.  The last call to le_ftpClient_Send() must set the done parameter to indicate the end of
 * the file.  Once the upload is complete, an LE_FTP_CLIENT_EVENT_DATAEND event will be generated.
 *
 * For a retrieval operation, the action is started by calling le_ftpClient_Retrieve().  As data is
 * received it will be passed to the le_ftpClient_WriteFunc_t callback, which executes in the event
 * loop of the thread that opened the session.  Once the download completes, an
 * LE_FTP_CLIENT_EVENT_DATAEND event will be generated.
 *
 * Any open session may be queried for its current status using the le_ftpClient_GetInfo()
 * function.  If no event callback has been specified, then this is the only means of determining if
 * an asynchronous operation has completed.
 *
 * @section ftp_client_example Example
 *
 * This example is derived from the test code found at
 * `$LEGATO_ROOT/components/test/ftpClientLibrary`.  It demonstrates opening a session and
 * connection, downloading a file, and destroying the session.  For clarity in the example, full
 * error checking has been replaced with calls to LE_ASSERT().
 *
 * @code
static void WriteFunc
(
    const void  *dataPtr,
    size_t       length,
    void        *writePtr
)
{
    // Write the received bytes out to the file.
    size_t result = fwrite(dataPtr, 1, length, writePtr);
    LE_ASSERT(result == length);
}

static void EventHandler
(
    le_ftpClient_SessionRef_t    sessionRef,
    le_ftpClient_Event_t         event,
    le_result_t                  result,
    void                        *userPtr
)
{
    FILE *outFile = userPtr;

    LE_ASSERT(event == LE_FTP_CLIENT_EVENT_DATAEND);
    LE_ASSERT(result == LE_OK);
    LE_ASSERT(outFile != NULL);

    fclose(outFile);

    // Close the connection and destroy the session.
    le_ftpClient_DestroySession(sessionRef);
}

COMPONENT_INIT
{
    FILE                        *outFile;
    le_ftpClient_SessionRef_t    sessionRef;
    le_result_t                  result;

    // Create the session instance and configure the remote server information.
    sessionRef = le_ftpClient_CreateSession(FTP_SERVER, LE_FTP_CLIENT_DEFAULT_CTRL_PORT, FTP_USER,
        FTP_PASS, FTP_TIMEOUT);
    LE_ASSERT(sessionRef != NULL);

    // Set up the completion callback.
    outFile = fopen(LOCAL_PATH, "w");
    LE_ASSERT(outFile != NULL);
    result = le_ftpClient_SetEventCallback(sessionRef, &EventHandler, outFile);
    LE_ASSERT(result == LE_OK);

    // Initiate the connection.
    result = le_ftpClient_Connect(sessionRef);
    LE_ASSERT(result == LE_OK);

    // Start downloading the file.
    result = le_ftpClient_Retrieve(sessionRef, REMOTE_PATH, LE_FTP_CLIENT_TRANSFER_BINARY, 0,
        &WriteFunc, outFile);
    LE_ASSERT(result == LE_OK);
}
 * @endcode
 */

/**
 * @file le_ftpClient.h
 *
 * FTP client library header file.
 * Data is transferred via callbacks to read and write the file content.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_FTP_CLIENT_H
#define LE_FTP_CLIENT_H

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 *  Default FTP control port.
 */
//--------------------------------------------------------------------------------------------------
#define LE_FTP_CLIENT_DEFAULT_CTRL_PORT 21U

//--------------------------------------------------------------------------------------------------
/**
 *  Transfer types.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_FTP_CLIENT_TRANSFER_BINARY,  ///< Binary transfer type.
    LE_FTP_CLIENT_TRANSFER_ASCII    ///< ASCII transfer type (not implemented).
} le_ftpClient_TransferType_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Connection mode.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_FTP_CLIENT_CONNECTION_PASSIVE,  ///< Passive FTP mode.
    LE_FTP_CLIENT_CONNECTION_ACTIVE    ///< Active FTP mode (not implemented).
} le_ftpClient_Mode_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Asynchronous FTP events.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_FTP_CLIENT_EVENT_NONE,       ///< No event.
    LE_FTP_CLIENT_EVENT_CLOSED,     ///< FTP connection closed asynchronously.
    LE_FTP_CLIENT_EVENT_TIMEOUT,    ///< Connection timed out.
    LE_FTP_CLIENT_EVENT_ERROR,      ///< Asynchronous error.
    LE_FTP_CLIENT_EVENT_DATA,       ///< Buffer of file data to transmit or receive.
    LE_FTP_CLIENT_EVENT_DATAEND,    ///< End of data.
    LE_FTP_CLIENT_EVENT_MEMORY_FREE ///< Memory available to send more data
} le_ftpClient_Event_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Session information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char          *serverStr;     ///< Host name or IP address of target FTP server.
    uint16_t             port;          ///< FTP server control port.
    int                  addressFamily; ///< Connection address family.
    const char          *userStr;       ///< User name used to log in to the server.
    le_ftpClient_Mode_t  mode;          ///< Connection mode.
    bool                 isConnected;   ///< Connection currently open.
    bool                 isRunning;     ///< Is an asynchronous operation (store/retrieve) running?
    unsigned int         response;      ///< FTP response code for the last operation.
} le_ftpClient_Info_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference to the FTP client context.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_ftpClient_Session *le_ftpClient_SessionRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Callback to write out a portion of a downloaded file.
 *
 *  @attention  Do not block in this function.  This function may be called from another thread.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_ftpClient_WriteFunc_t)
(
    const void  *dataPtr,   ///< Downloaded data chunk.
    size_t       length,    ///< Length of downloaded data in bytes.
    void        *writePtr   ///< User pointer that was passed to le_ftpClient_Retrieve().
);

//--------------------------------------------------------------------------------------------------
/**
 *  Callback to indicate an asynchronous session event has occurred.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_ftpClient_EventFunc_t)
(
    le_ftpClient_SessionRef_t    sessionRef,    ///< Session reference.
    le_ftpClient_Event_t         event,         ///< Event which occurred.
    le_result_t                  result,        ///< Legato result code corresponding to the event.
    void                        *userPtr        ///< User data that was associated at callback
                                                ///  registration.
);

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
LE_SHARED le_ftpClient_SessionRef_t le_ftpClient_CreateSession
(
    const char      *serverStr,     ///< Host name or IP address of target FTP server.
    uint16_t         port,          ///< FTP server control port.  Use
                                    ///  LE_FTP_CLIENT_DEFAULT_CTRL_PORT for the default.
    const char      *userStr,       ///< User name to log into the server under.
    const char      *passwordStr,   ///< Password to use when logging in to the server.
    unsigned int     timeout        ///< Connection timeout in seconds.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Close and destroy an FTP client session.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_ftpClient_DestroySession
(
    le_ftpClient_SessionRef_t sessionRef    ///< Session reference.
);

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
    le_ftpClient_EventFunc_t     handlerFunc,   ///< Handler callback.
    void                        *userPtr        ///< Additional data to pass to the handler.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Open a new connection to the configured server.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_ftpClient_Connect
(
    le_ftpClient_SessionRef_t sessionRef    ///< Session reference.
);

LE_SHARED le_result_t le_ftpClient_ConnectOnSrcAddr
(
    le_ftpClient_SessionRef_t sessionRef,   ///< Session reference.
    char*                     srcAddr       ///< [IN] Source Address of PDP profile.
);
//--------------------------------------------------------------------------------------------------
/**
 *  Close the active connection.  A new connection may be opened with le_ftpClient_Connect().
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_ftpClient_Disconnect
(
    le_ftpClient_SessionRef_t sessionRef    ///< Session reference.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Retrieve a file from the remote server.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_ftpClient_Retrieve
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
);

//--------------------------------------------------------------------------------------------------
/**
 *  Upload a file to the remote server.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_ftpClient_Store
(
    le_ftpClient_SessionRef_t    sessionRef,    ///< Session reference.
    const char                  *pathStr,       ///< File path on remote server.
    le_ftpClient_TransferType_t  type,          ///< Transfer type.
    bool                         append         ///< Append to the remote file, rather than
                                                ///  overwriting it.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Send some file data to the remote server.  A store operation must be active when this function
 *  is called.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_ftpClient_Send
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
);

//--------------------------------------------------------------------------------------------------
/**
 *  Delete a file from the remote server.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_ftpClient_Delete
(
    le_ftpClient_SessionRef_t    sessionRef,    ///< Session reference.
    const char                  *pathStr        ///< File path on remote server.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Query the size of a file on the remote server.  This can be used for determining the appropriate
 *  data to upload when resuming a Store command.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_ftpClient_Size
(
    le_ftpClient_SessionRef_t    sessionRef,    ///< [IN]  Session reference.
    const char                  *pathStr,       ///< [IN]  File path on remote server.
    le_ftpClient_TransferType_t  type,          ///< [IN]  Transfer type.
    uint64_t                    *sizePtr        ///< [OUT] Size of remote file in bytes.
);

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

);

#endif /* end LE_FTP_CLIENT_H */
