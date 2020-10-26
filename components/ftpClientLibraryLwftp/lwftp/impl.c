/**
 * @file impl.c
 *
 * Implementation of Legato FTP client built on top of lwftp.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "impl.h"
#include "../le_ftpClient.h"
#include "legato.h"
#include "interfaces.h"

#include "lwftp.h"
#include "lwip/api.h"

#include <sys/socket.h>

//--------------------------------------------------------------------------------------------------
/**
 * Determine minimum of two values.
 *
 * @param   a   First value.
 * @param   b   Second value.
 *
 * @return  Minimum of the two values.
 */
//--------------------------------------------------------------------------------------------------
#define MIN(a, b)   (((a) < (b)) ? (a) : (b))

/// Allocate a data buffer when allocating an event instance.
#define EVT_WITH_BUFFER 0x1U
/// Block until an instance is available when trying to allocate an event instance.
#define EVT_BLOCK       0x2U
/// Force allocation of an event by expanding the memory pool if necessary.
#define EVT_FORCE       0x4U

/// FTP response code for successful login.
#define RESP_LOGGED_IN  230

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
    bool                         needsResume;           ///< Trigger lwftp resume on next send.
    le_event_HandlerRef_t        eventHandlerRef;       ///< Event handler for asynchronous events.
    le_ftpClient_EventFunc_t     eventHandlerFunc;      ///< Asynchronous event handler callback.

    le_mutex_Ref_t               mutexRef;              ///< Event queue mutex.
    le_sls_List_t                eventQueue;            ///< Event queue.
    void                        *eventHandlerDataPtr;   ///< User data to pass to event handler.

    lwftp_session_t              lwftp;         ///< lwftp session instance.
};

//--------------------------------------------------------------------------------------------------
/**
 *  Container for passing data between threads.
 */
//--------------------------------------------------------------------------------------------------
struct DataBuffer
{
    size_t  length;                                 ///< Length of the data, if any.
    size_t  offset;                                 ///< Offset to indicate data that has already
                                                    ///< processed.
    char    data[LE_CONFIG_FTPCLIENT_BUFFER_SIZE];  ///< Buffer data.
};

//--------------------------------------------------------------------------------------------------
/**
 *  Container for maintaining reference counting of sessions in queued asynchronous events.
 */
//--------------------------------------------------------------------------------------------------
struct AsyncEvent
{
    union
    {
        le_sls_Link_t            link;          ///< Linked list entry.
        err_t                    result;        ///< Result code from lwftp.
    };
    le_ftpClient_SessionRef_t    sessionRef;    ///< Session associated with the event.
    le_ftpClient_Event_t         event;         ///< Event code.
    struct DataBuffer           *bufferPtr;     ///< Associated data buffer, if any.
};

//--------------------------------------------------------------------------------------------------
/**
 *  Container for EventId for each session.
 */
//--------------------------------------------------------------------------------------------------
struct EventIdInfo
{
    le_event_Id_t  EventId;
    bool           active;
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
 *  Memory pool for asynchronous events.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t EventPool;
LE_MEM_DEFINE_STATIC_POOL(Event, LE_CONFIG_FTPCLIENT_EVENT_MAX, sizeof(struct AsyncEvent));

//--------------------------------------------------------------------------------------------------
/**
 *  Memory pool for data buffers.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t BufferPool;
LE_MEM_DEFINE_STATIC_POOL(Buffer, LE_CONFIG_FTPCLIENT_EVENT_MAX, sizeof(struct DataBuffer));

//--------------------------------------------------------------------------------------------------
/**
 *  Event ID for asynchronous FTP events.
 */
//--------------------------------------------------------------------------------------------------
static struct EventIdInfo EventId[LE_CONFIG_FTPCLIENT_SESSION_MAX];

//--------------------------------------------------------------------------------------------------
/**
 *  Mutex for changing the state of EventId.
 */
//--------------------------------------------------------------------------------------------------
static le_mutex_Ref_t FtpMutexRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 *  Semaphore for counting free event objects.
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t EventPoolSemRef;

//--------------------------------------------------------------------------------------------------
/**
 *  Convert a lwftp result code to a string for debugging.
 *
 *  @return lwftp result string.
 */
//--------------------------------------------------------------------------------------------------
static const char *LwftpResultString
(
    int result ///< lwftp result code to translate.
)
{
    switch (result)
    {
        case LWFTP_RESULT_OK:
            return "LWFTP_RESULT_OK";
        case LWFTP_RESULT_INPROGRESS:
            return "LWFTP_RESULT_INPROGRESS";
        case LWFTP_RESULT_LOGGED:
            return "LWFTP_RESULT_LOGGED";
        case LWFTP_RESULT_ERR_UNKNOWN:
            return "LWFTP_RESULT_ERR_UNKNOWN";
        case LWFTP_RESULT_ERR_ARGUMENT:
            return "LWFTP_RESULT_ERR_ARGUMENT";
        case LWFTP_RESULT_ERR_MEMORY:
            return "LWFTP_RESULT_ERR_MEMORY";
        case LWFTP_RESULT_ERR_CONNECT:
            return "LWFTP_RESULT_ERR_CONNECT";
        case LWFTP_RESULT_ERR_HOSTNAME:
            return "LWFTP_RESULT_ERR_HOSTNAME";
        case LWFTP_RESULT_ERR_CLOSED:
            return "LWFTP_RESULT_ERR_CLOSED";
        case LWFTP_RESULT_ERR_TIMEOUT:
            return "LWFTP_RESULT_ERR_TIMEOUT";
        case LWFTP_RESULT_ERR_SRVR_RESP:
            return "LWFTP_RESULT_ERR_SRVR_RESP";
        case LWFTP_RESULT_ERR_INTERNAL:
            return "LWFTP_RESULT_ERR_INTERNAL";
        case LWFTP_RESULT_ERR_LOCAL:
            return "LWFTP_RESULT_ERR_LOCAL";
        case LWFTP_RESULT_ERR_FILENAME:
            return "LWFTP_RESULT_ERR_FILENAME";
    }

    return "Unknown";
}

//--------------------------------------------------------------------------------------------------
/**
 *  Convert an event enum value to a string for debugging.
 *
 *  @return Event string.
 */
//--------------------------------------------------------------------------------------------------
static const char *EventString
(
    le_ftpClient_Event_t    event
)
{
    switch (event)
    {
        case LE_FTP_CLIENT_EVENT_NONE:
            return "LE_FTP_CLIENT_EVENT_NONE";
        case LE_FTP_CLIENT_EVENT_CLOSED:
            return "LE_FTP_CLIENT_EVENT_CLOSED";
        case LE_FTP_CLIENT_EVENT_TIMEOUT:
            return "LE_FTP_CLIENT_EVENT_TIMEOUT";
        case LE_FTP_CLIENT_EVENT_ERROR:
            return "LE_FTP_CLIENT_EVENT_ERROR";
        case LE_FTP_CLIENT_EVENT_DATA:
            return "LE_FTP_CLIENT_EVENT_DATA";
        case LE_FTP_CLIENT_EVENT_DATAEND:
            return "LE_FTP_CLIENT_EVENT_DATAEND";
        case LE_FTP_CLIENT_EVENT_MEMORY_FREE:
            return "LE_FTP_CLIENT_EVENT_MEMORY_FREE";
    }

    return "Unknown";
}

//--------------------------------------------------------------------------------------------------
/**
 *  Peek at the head of the event queue.
 *
 *  @return
 *      -   NULL if the queue is empty.
 *      -   The event instance at the head of the queue.
 */
//--------------------------------------------------------------------------------------------------
static struct AsyncEvent *PeekEvent
(
    le_ftpClient_SessionRef_t sessionRef    ///< Session instance.
)
{
    return (struct AsyncEvent *) le_sls_Peek(&sessionRef->eventQueue);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send an event to the local queue.
 */
//--------------------------------------------------------------------------------------------------
static void SendEvent
(
    struct AsyncEvent *eventPtr ///< Event to send.
)
{
    err_t                       error;
    le_ftpClient_SessionRef_t   sessionRef = eventPtr->sessionRef;

    LE_DEBUG("Sending event %s", EventString(eventPtr->event));

    eventPtr->link = LE_SLS_LINK_INIT;
    le_mutex_Lock(sessionRef->mutexRef);
    le_sls_Queue(&sessionRef->eventQueue, &eventPtr->link);
    if (sessionRef->needsResume)
    {
        error = lwftp_resume_send(&sessionRef->lwftp);
        if (error != LWFTP_RESULT_OK)
        {
            LE_ERROR("Error resuming FTP send: %s", LwftpResultString(error));
        }
    }
    le_mutex_Unlock(sessionRef->mutexRef);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Create a new asynchronous event instance.  If the EVT_BLOCK flag is set, blocks until an event
 *  can be allocated.
 *
 *  @return New event instance, or NULL if not blocking and no instance is available.
 */
//--------------------------------------------------------------------------------------------------
static struct AsyncEvent *NewEvent
(
    le_ftpClient_SessionRef_t   sessionRef, ///< Session instance.
    unsigned int                flags       ///< Settings for allocating the event.
)
{
    struct AsyncEvent *eventPtr;

    // Wait for an event object to become available.
    if (flags & EVT_BLOCK)
    {
        le_sem_Wait(EventPoolSemRef);
    }
    else if ((flags & EVT_FORCE) == 0)
    {
        if (le_sem_TryWait(EventPoolSemRef) != LE_OK)
        {
            return NULL;
        }
    }

    // If the semaphore was signalled, there must be an event object available now, or momentarily
    // if the destuctor is in progress.
    if (flags & EVT_FORCE)
    {
        eventPtr = le_mem_ForceAlloc(EventPool);
    }
    else
    {
        eventPtr = le_mem_TryAlloc(EventPool);
        if(eventPtr == NULL)
        {
            return NULL;
        }
    }
    memset(eventPtr, 0, sizeof(*eventPtr));

    eventPtr->sessionRef = sessionRef;
    le_mem_AddRef(sessionRef);

    if (flags & EVT_WITH_BUFFER)
    {
        // If we were able to get an event object, there must be a buffer object available now, or
        // momentarily if the destuctor is in progress.
        if (flags & EVT_FORCE)
        {
            eventPtr->bufferPtr = le_mem_ForceAlloc(BufferPool);
        }
        else
        {
            eventPtr->bufferPtr = le_mem_TryAlloc(BufferPool);
            if (eventPtr->bufferPtr == NULL)
            {
                // unable to allocate buffer for this event, therefore event must be released
                le_mem_Release(eventPtr);
                return NULL;
            }
        }
        memset(eventPtr->bufferPtr, 0, sizeof(*eventPtr->bufferPtr));
    }
    return eventPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Pop and free the item from the head of the event queue.
 */
//--------------------------------------------------------------------------------------------------
static void PopEvent
(
    le_ftpClient_SessionRef_t sessionRef    ///< Session instance.
)
{
    le_mem_PoolStats_t  stats;
    struct AsyncEvent   *eventPtr = (struct AsyncEvent *) le_sls_Pop(&sessionRef->eventQueue);

    le_mem_Release(eventPtr);

    le_mem_GetStats(EventPool, &stats);
    if (stats.numFree >= 2)
    {
        eventPtr = NewEvent(sessionRef, 0);
        if (eventPtr != NULL)
        {
            eventPtr->result = LWFTP_RESULT_OK;
            eventPtr->event = LE_FTP_CLIENT_EVENT_MEMORY_FREE;
            le_event_ReportWithRefCounting(EventId[sessionRef->eventIdIndex].EventId, eventPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Read data from the source for upload.
 *
 *  @return
 *      -   Greater than 0 for the number of bytes read.
 *      -   0 to indicate the end of the file.
 *      -   Less than 0 to indicate that more data is to come, but none is available at this time.
 */
//--------------------------------------------------------------------------------------------------
static int SourceData
(
    void         *paramPtr, ///< [IN]    Session instance.
    const char  **dataPtr,  ///< [INOUT] Set to location of data to upload.
    uint          maxLength ///< [IN]    Maximum number of bytes to upload.
)
{
    le_ftpClient_SessionRef_t    sessionRef = paramPtr;
    ssize_t                      length = 0;
    struct AsyncEvent           *bufferEventPtr;
    struct DataBuffer           *bufferPtr;

    le_mutex_Lock(sessionRef->mutexRef);

    // Handle item on top of the buffer queue.
    bufferEventPtr = PeekEvent(sessionRef);

    if (dataPtr == NULL)
    {
        if (bufferEventPtr != NULL)
        {
            if (bufferEventPtr->event == LE_FTP_CLIENT_EVENT_DATA)
            {
                // A data buffer is available.
                bufferPtr = bufferEventPtr->bufferPtr;
                LE_ASSERT(bufferPtr != NULL);

                // Track the number of bytes actually sent.
                bufferPtr->offset += maxLength;
                bufferPtr->length -= MIN(maxLength, bufferPtr->length);
                if (bufferPtr->length == 0)
                {
                    // Only pop the buffer off the queue if we have sent all of its bytes.
                    PopEvent(sessionRef);
                }
            }
        }
    }
    else
    {
        // Call is requesting more data to transfer.
        LE_ASSERT(sessionRef->operation == OP_STORE);

        if (bufferEventPtr == NULL)
        {
            // No item, so indicate to lwftp that we need to try again later.
            length = -1;
        }
        else if (bufferEventPtr->event == LE_FTP_CLIENT_EVENT_DATA)
        {
            // A data buffer is available.
            bufferPtr = bufferEventPtr->bufferPtr;

            LE_ASSERT(bufferPtr != NULL);
            LE_ASSERT(bufferPtr->length > 0);

            // Send as many bytes as we can.
            *dataPtr = bufferPtr->data + bufferPtr->offset;
            length = MIN(maxLength, bufferPtr->length);
        }
        else if (bufferEventPtr->event == LE_FTP_CLIENT_EVENT_DATAEND)
        {
            // No more data to send.
            LE_ASSERT(bufferEventPtr->bufferPtr == NULL);
            PopEvent(sessionRef);
        }
    }

    sessionRef->needsResume = (length < 0);
    le_mutex_Unlock(sessionRef->mutexRef);
    return length;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Process a response from the server.  This could be downloaded data or other information such as
 *  file size.
 *
 *  @return Number of bytes processed.
 */
//--------------------------------------------------------------------------------------------------
static uint SinkData
(
    void        *paramPtr,  ///< Session instance.
    const char  *dataPtr,   ///< Downloaded data.
    uint         size       ///< Number of bytes available.
)
{
    le_ftpClient_SessionRef_t    sessionRef = paramPtr;
    size_t                       length = 0;
    struct AsyncEvent           *eventPtr;
    struct DataBuffer           *bufferPtr;

    if (dataPtr != NULL)
    {
        // Call is providing more data.
        switch (sessionRef->operation)
        {
            case OP_RETRIEVE:
                while (size > 0)
                {
                    eventPtr = NewEvent(sessionRef, EVT_WITH_BUFFER);
                    if (eventPtr == NULL)
                    {
                        sessionRef->needsResume = true;
                        return length;
                    }

                    eventPtr->event = LE_FTP_CLIENT_EVENT_DATA;
                    bufferPtr = eventPtr->bufferPtr;

                    bufferPtr->length = MIN(LE_CONFIG_FTPCLIENT_BUFFER_SIZE, size);
                    memcpy(bufferPtr->data, dataPtr, bufferPtr->length);

                    size -= bufferPtr->length;
                    dataPtr += bufferPtr->length;
                    length += bufferPtr->length;
                    le_event_ReportWithRefCounting(EventId[sessionRef->eventIdIndex].EventId, eventPtr);
                }
                break;
            case OP_SIZE:
                LE_ASSERT(size == sizeof(sessionRef->fileSize));
                sessionRef->fileSize = *((const uint64_t *) dataPtr);
                length = size;
                break;
            default:
                break;
        }
    }
    return length;
}

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
 *  Handle completion (or failure) of a blocking operation.  This unblocks the function which
 *  initiated the request.
 */
//--------------------------------------------------------------------------------------------------
static void HandleBlockingResult
(
    le_ftpClient_SessionRef_t   sessionRef, ///< Session instance.
    int                         result      ///< lwftp result code.
)
{
    if (result != LWFTP_RESULT_INPROGRESS)
    {
        sessionRef->result = result;
        le_sem_Post(sessionRef->semRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Handle completion (or failure) of a non-blocking operation.
 */
//--------------------------------------------------------------------------------------------------
static void HandleNonBlockingResult
(
    le_ftpClient_SessionRef_t   sessionRef, ///< Session instance.
    int                         result      ///< lwftp result code.
)
{
    le_ftpClient_Event_t     event = LE_FTP_CLIENT_EVENT_ERROR;
    struct AsyncEvent       *eventPtr;

    switch (result)
    {
        case LWFTP_RESULT_INPROGRESS:
            // Ignore in-progress messages.
            return;
        case LWFTP_RESULT_ERR_CLOSED:
            // Asynchronous close (e.g. server closed control connection).
            event = LE_FTP_CLIENT_EVENT_CLOSED;
            break;
        case LWFTP_RESULT_ERR_TIMEOUT:
            // Asynchronous timeout.
            event = LE_FTP_CLIENT_EVENT_TIMEOUT;
            break;
        case LWFTP_RESULT_OK:
            // End of data for operation.
            event = LE_FTP_CLIENT_EVENT_DATAEND;
            break;
    }

    eventPtr = NewEvent(sessionRef, EVT_FORCE);
    LE_ASSERT(eventPtr!=NULL);
    eventPtr->event = event;
    eventPtr->result = result;
    le_event_ReportWithRefCounting(EventId[sessionRef->eventIdIndex].EventId, eventPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Handle completion (or failure) of an operation.
 */
//--------------------------------------------------------------------------------------------------
static void HandleResult
(
    void    *paramPtr,  ///< Session instance.
    int      result     ///< lwftp result code.
)
{
    le_ftpClient_SessionRef_t sessionRef = paramPtr;

    if (IsBlocking(sessionRef->operation))
    {
        HandleBlockingResult(sessionRef, result);
    }
    else
    {
        HandleNonBlockingResult(sessionRef, result);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Convert a lwftp result code into a Legato result code.
 *
 *  @return
 */
//--------------------------------------------------------------------------------------------------
static inline le_result_t TranslateLwftpResult
(
    int value   ///< Result code from lwftp.
)
{
    le_result_t result = LE_FAULT;

    switch (value)
    {
        case LWFTP_RESULT_OK:
        case LWFTP_RESULT_LOGGED:
            result = LE_OK;
            break;
        case LWFTP_RESULT_ERR_UNKNOWN:
            result = LE_FAULT;
            break;
        case LWFTP_RESULT_ERR_ARGUMENT:
            result = LE_BAD_PARAMETER;
            break;
        case LWFTP_RESULT_ERR_MEMORY:
            result = LE_NO_MEMORY;
            break;
        case LWFTP_RESULT_ERR_CONNECT:
        case LWFTP_RESULT_ERR_CLOSED:
            result = LE_CLOSED;
            break;
        case LWFTP_RESULT_ERR_SRVR_RESP:
            result = LE_COMM_ERROR;
            break;
        case LWFTP_RESULT_ERR_FILENAME:
            result = LE_NOT_FOUND;
            break;
        case LWFTP_RESULT_ERR_TIMEOUT:
            result = LE_TIMEOUT;
            break;
        default:
            break;
    }
    LE_DEBUG("%s -> %s", LwftpResultString(value), LE_RESULT_TXT(result));

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Handle an asynchronous session event.
 */
//--------------------------------------------------------------------------------------------------
static void EventHandler
(
    struct AsyncEvent *eventPtr ///< Pointer to event object.
)
{
    le_ftpClient_SessionRef_t    sessionRef = eventPtr->sessionRef;
    struct DataBuffer           *bufferPtr;

    switch (eventPtr->event)
    {
        case LE_FTP_CLIENT_EVENT_CLOSED:
        case LE_FTP_CLIENT_EVENT_TIMEOUT:
            sessionRef->isConnected = false;
            break;
        case LE_FTP_CLIENT_EVENT_DATA:
            LE_ASSERT(sessionRef->operation == OP_RETRIEVE);

            bufferPtr = eventPtr->bufferPtr;
            LE_ASSERT(eventPtr->bufferPtr != NULL);

            sessionRef->writeFunc(bufferPtr->data, bufferPtr->length, sessionRef->userDataPtr);
            le_mem_Release(eventPtr);
            return;
        case LE_FTP_CLIENT_EVENT_DATAEND:
            sessionRef->operation = OP_NONE;
            sessionRef->needsResume = false;
            break;
        case LE_FTP_CLIENT_EVENT_MEMORY_FREE:
            break;
        default:
            break;
    }

    if (sessionRef->eventHandlerFunc != NULL)
    {
        sessionRef->eventHandlerFunc(
            sessionRef,
            eventPtr->event,
            TranslateLwftpResult(eventPtr->result),
            sessionRef->eventHandlerDataPtr
        );
    }
    le_mem_Release(eventPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Wait for a lwftp operation to complete, and generate an appropriate Legato result.
 *
 *  @return Legato result code corresponding to lwftp result code.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WaitForResult
(
    le_ftpClient_SessionRef_t   sessionRef  ///< Session instance.
)
{
    le_sem_Wait(sessionRef->semRef);
    return TranslateLwftpResult(sessionRef->result);
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
    le_sls_Link_t *l;

    le_ftpClient_SetEventCallback(sessionRef, NULL, NULL);
    le_ftpClient_Disconnect(sessionRef);
    memset(sessionRef->passwordStr, 0, sizeof(sessionRef->passwordStr));
    le_sem_Delete(sessionRef->semRef);
    le_mutex_Delete(sessionRef->mutexRef);

    for (l = le_sls_Pop(&sessionRef->eventQueue);
         l != NULL;
         l = le_sls_Pop(&sessionRef->eventQueue))
    {
        le_mem_Release(l);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Destructor for asynchronous events.
 */
//--------------------------------------------------------------------------------------------------
static void EventDestructor
(
    struct AsyncEvent *eventPtr ///< Event to destroy.
)
{
    err_t                       error;
    le_ftpClient_SessionRef_t   sessionRef = eventPtr->sessionRef;
    bool                        resume = (sessionRef->operation == OP_RETRIEVE &&
                                    sessionRef->needsResume && eventPtr->bufferPtr != NULL);
    le_mem_Release(sessionRef);
    if (eventPtr->bufferPtr != NULL)
    {
        le_mem_Release(eventPtr->bufferPtr);
    }

    le_sem_Post(EventPoolSemRef);
    if (resume)
    {
        error = lwftp_resume_recv(&sessionRef->lwftp);
        if (error != LWFTP_RESULT_OK)
        {
            LE_ERROR("Error resuming FTP receive: %s", LwftpResultString(error));
        }
    }
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

    EventPool = le_mem_InitStaticPool(Event, LE_CONFIG_FTPCLIENT_EVENT_MAX, sizeof(struct AsyncEvent));
    le_mem_SetDestructor(EventPool, (le_mem_Destructor_t) &EventDestructor);

    EventPoolSemRef = le_sem_Create("EventPoolSem", LE_CONFIG_FTPCLIENT_EVENT_MAX);

    BufferPool = le_mem_InitStaticPool(Buffer, LE_CONFIG_FTPCLIENT_EVENT_MAX,
                    sizeof(struct DataBuffer));

    FtpMutexRef = le_mutex_CreateNonRecursive("FtpMutexRef");
    int i;
    for (i = 0; i < LE_CONFIG_FTPCLIENT_SESSION_MAX; i++)
    {
        EventId[i].EventId = le_event_CreateIdWithRefCounting("FTPClientEvent");
        EventId[i].active = false;
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
        LE_WARN("Invalid server");
        return NULL;
    }
    if (userStr == NULL ||
        strlen(userStr) >= LE_CONFIG_FTPCLIENT_USER_NAME_MAX)
    {
        LE_WARN("Invalid username");
        return NULL;
    }
    if (passwordStr == NULL ||
        strlen(passwordStr) >= LE_CONFIG_FTPCLIENT_PASSWORD_MAX)
    {
        LE_WARN("Invalid password");
        return NULL;
    }

    // Allocate the session
    sessionRef = le_mem_TryAlloc(SessionPool);
    if (sessionRef == NULL)
    {
        LE_ERROR("No more sessions available");
        return NULL;
    }
    memset(sessionRef, 0, sizeof(*sessionRef));

    // Populate the session
    int i;
    le_mutex_Lock(FtpMutexRef);
    for (i = 0; i < LE_CONFIG_FTPCLIENT_SESSION_MAX; i++)
    {
        if (EventId[i].active == false) // Inactive Id found. Using Id for current session
        {
            EventId[i].active = true;
            break;
        }
    }
    le_mutex_Unlock(FtpMutexRef);

    strlcpy(sessionRef->serverStr, serverStr, sizeof(sessionRef->serverStr));
    strlcpy(sessionRef->userStr, userStr, sizeof(sessionRef->userStr));
    strlcpy(sessionRef->passwordStr, passwordStr, sizeof(sessionRef->passwordStr));
    sessionRef->operation = OP_NONE;
    sessionRef->semRef = le_sem_Create(serverStr, 0);
    sessionRef->mutexRef = le_mutex_CreateNonRecursive(serverStr);
    sessionRef->eventQueue = LE_SLS_LIST_INIT;
    sessionRef->eventIdIndex = i;
    sessionRef->eventHandlerRef = le_event_AddHandler(serverStr,
                                                     EventId[i].EventId,
                                (le_event_HandlerFunc_t) &EventHandler);
    sessionRef->lwftp.server_port = port;
    sessionRef->lwftp.user = sessionRef->userStr;
    sessionRef->lwftp.pass = sessionRef->passwordStr;
    sessionRef->lwftp.handle = sessionRef;
    sessionRef->lwftp.data_source = &SourceData;
    sessionRef->lwftp.data_sink = &SinkData;
    sessionRef->lwftp.done_fn = &HandleResult;
    sessionRef->lwftp.timeout = timeout * 1000;

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
    le_event_RemoveHandler(sessionRef->eventHandlerRef);
    EventId[sessionRef->eventIdIndex].active = false;
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
    LE_ERROR("FTPS is not supported on lwip-based products");
    return LE_UNSUPPORTED;
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
    char        serverBuf[46];
    err_t       err;
    le_result_t result;

    if (sessionRef == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    if (sessionRef->isConnected)
    {
        result = LE_OK;
    }
    else
    {
        err = netconn_gethostbyname(sessionRef->serverStr, &sessionRef->lwftp.server_ip);
        if (err != ERR_OK)
        {
            LE_ERROR("Name resolution of FTP server failed: %d (%s)", err, lwip_strerr(err));

            // Return unavailable since the name cannot currently be resolved.
            return LE_UNAVAILABLE;
        }

        ipaddr_ntoa_r(&sessionRef->lwftp.server_ip, serverBuf, sizeof(serverBuf));
        LE_INFO("Using FTP server %s, port %u", serverBuf, sessionRef->lwftp.server_port);

        // Start operation.
        sessionRef->operation = OP_CONNECT;
        lwftp_connect(&sessionRef->lwftp);

        // Wait for completion.
        result = WaitForResult(sessionRef);
        if (result == LE_OK)
        {
            // Check FTP response code, too.
            result = (sessionRef->lwftp.response == RESP_LOGGED_IN ? LE_OK : LE_COMM_ERROR);
        }
        sessionRef->operation = OP_NONE;
        sessionRef->isConnected = (result == LE_OK);
    }

    return result;
}

le_result_t le_ftpClient_ConnectOnSrcAddr
(
    le_ftpClient_SessionRef_t sessionRef,   ///< Session reference.
    char*                     srcAddr       ///< [IN] Source Address of PDP profile.
)
{
    return le_ftpClient_Connect(sessionRef);
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
        // Start operation.
        sessionRef->operation = OP_DISCONNECT;
        lwftp_close(&sessionRef->lwftp);

        // Wait for completion.
        WaitForResult(sessionRef);
        sessionRef->isConnected = false;
        sessionRef->operation = OP_NONE;
    }
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
    if (sessionRef == NULL || pathStr == NULL || writeFunc == NULL)
    {
        return LE_BAD_PARAMETER;
    }
    if (type != LE_FTP_CLIENT_TRANSFER_BINARY)
    {
        return LE_NOT_IMPLEMENTED;
    }

    // Start operation.
    sessionRef->operation = OP_RETRIEVE;
    sessionRef->needsResume = false;
    sessionRef->lwftp.remote_path = pathStr;
    sessionRef->lwftp.restart = offset;
    sessionRef->writeFunc = writeFunc;
    sessionRef->userDataPtr = writePtr;
    lwftp_retrieve(&sessionRef->lwftp);

    return LE_OK;
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
    if (sessionRef == NULL || pathStr == NULL)
    {
        return LE_BAD_PARAMETER;
    }
    if (type != LE_FTP_CLIENT_TRANSFER_BINARY)
    {
        return LE_NOT_IMPLEMENTED;
    }

    // Start operation.
    sessionRef->operation = OP_STORE;
    sessionRef->needsResume = false;
    sessionRef->lwftp.remote_path = pathStr;
    if (append)
    {
        lwftp_append(&sessionRef->lwftp);
    }
    else
    {
        lwftp_store(&sessionRef->lwftp);
    }

    return LE_OK;
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
    struct AsyncEvent   *eventPtr;
    struct DataBuffer   *bufferPtr;

    if (sessionRef == NULL || sessionRef->operation != OP_STORE)
    {
        return LE_BAD_PARAMETER;
    }

    if (*length > 0)
    {
        eventPtr = NewEvent(sessionRef, EVT_WITH_BUFFER);
        if (eventPtr == NULL)
        {
            return LE_NO_MEMORY;
        }
        eventPtr->event = LE_FTP_CLIENT_EVENT_DATA;
        bufferPtr = eventPtr->bufferPtr;

        bufferPtr->length = MIN(LE_CONFIG_FTPCLIENT_BUFFER_SIZE, *length);
        memcpy(bufferPtr->data, dataPtr, bufferPtr->length);

        *length -= bufferPtr->length;
        SendEvent(eventPtr);
    }

    if (*length == 0 && done)
    {
        eventPtr = NewEvent(sessionRef, 0);
        if (eventPtr == NULL)
        {
            return LE_NO_MEMORY;
        }
        eventPtr->event = LE_FTP_CLIENT_EVENT_DATAEND;
        SendEvent(eventPtr);
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
    le_result_t result;

    if (sessionRef == NULL || pathStr == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    // Start operation.
    sessionRef->operation = OP_DELETE;
    sessionRef->lwftp.remote_path = pathStr;
    lwftp_delete(&sessionRef->lwftp);

    // Wait for completion.
    result = WaitForResult(sessionRef);
    sessionRef->operation = OP_NONE;
    return result;
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
    le_result_t result;

    if (sessionRef == NULL || pathStr == NULL || sizePtr == NULL)
    {
        return LE_BAD_PARAMETER;
    }
    if (type != LE_FTP_CLIENT_TRANSFER_BINARY)
    {
        return LE_NOT_IMPLEMENTED;
    }

    // Start operation.
    sessionRef->operation = OP_SIZE;
    sessionRef->lwftp.remote_path = pathStr;
    lwftp_size(&sessionRef->lwftp);

    // Wait for completion.
    result = WaitForResult(sessionRef);
    if (result == LE_OK)
    {
        *sizePtr = sessionRef->fileSize;
    }
    sessionRef->operation = OP_NONE;
    return result;
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
    infoPtr->port = sessionRef->lwftp.server_port;
    infoPtr->addressFamily = (IP_IS_V4_VAL(sessionRef->lwftp.server_ip) ? AF_INET : AF_INET6);
    infoPtr->userStr = sessionRef->userStr;
    infoPtr->mode = LE_FTP_CLIENT_CONNECTION_PASSIVE;
    infoPtr->isConnected = sessionRef->isConnected;
    infoPtr->isRunning = (sessionRef->operation != OP_NONE);
    infoPtr->response = sessionRef->lwftp.response;
    return LE_OK;
}
