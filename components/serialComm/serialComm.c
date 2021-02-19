/**
 * @file
 *
 * This file implements the le_comm.h interface for use with a serial device.
 *
 *
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "legato.h"
#include "interfaces.h"
#include "le_comm.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum size of the serial device name
 */
//--------------------------------------------------------------------------------------------------
#define LE_COMM_SERIAL_DEVICE_NAME_STRLEN_MAX            20

//--------------------------------------------------------------------------------------------------
/**
 * Handshaking characters for le_comm_connect
 */
//--------------------------------------------------------------------------------------------------
#define LE_COMM_CONNECT_HELLO                             'X'
#define LE_COMM_CONNECT_HELLOACK                          'Y'

//--------------------------------------------------------------------------------------------------
/**
 * Connection time out timer duration in seconds
 */
//--------------------------------------------------------------------------------------------------
#ifdef LE_COMM_SERVER
#define LE_COMM_CONNECTION_TIMEOUT_TIMER_DURATION                120
#else
#define LE_COMM_CONNECTION_TIMEOUT_TIMER_DURATION                5
#endif

//--------------------------------------------------------------------------------------------------
/**
 * A type to keep track of the connection state
 */
//--------------------------------------------------------------------------------------------------
typedef enum connectionState
{
    LE_COMM_IDLE,
    LE_COMM_CONNECTING,
    LE_COMM_CONNECTED,
} connectionState_t;

//--------------------------------------------------------------------------------------------------
/**
 * A structure to hold variables related to a serial connection.
 */
//--------------------------------------------------------------------------------------------------
typedef struct HandleRecord
{
    le_fdMonitor_Ref_t fdMonitorRef; ///< Reference to the fd monitor
    int serialFd;                    ///< Serial device file descriptor
    connectionState_t connState;     ///< Internal state to hold the connect status
    char serialDeviceName[LE_COMM_SERIAL_DEVICE_NAME_STRLEN_MAX]; ///< Serial port name
}
HandleRecord_t;

//--------------------------------------------------------------------------------------------------
/**
 * Pointer type defined for handleRecord_t
 */
//--------------------------------------------------------------------------------------------------
typedef HandleRecord_t* HandleRecordPtr_t;


//--------------------------------------------------------------------------------------------------
/**
 * A static instance of HandleRecord_t
 */
//--------------------------------------------------------------------------------------------------
static HandleRecord_t SerialHandleRecord = {
    .fdMonitorRef = NULL,
    .serialFd = 0,
    .connState = LE_COMM_IDLE,
    .serialDeviceName = {0}
};


//--------------------------------------------------------------------------------------------------
/**
 * Registered Asynchronous Receive Callback Handler function
 */
//--------------------------------------------------------------------------------------------------
static le_comm_CallbackHandlerFunc_t AsyncReceiveHandlerFuncPtr = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Registered Asynchronous Connection Callback Handler function
 */
//--------------------------------------------------------------------------------------------------
static le_comm_CallbackHandlerFunc_t AsyncConnectionHandlerFuncPtr = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Component init for serialComm
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{

}

#ifdef LE_COMM_SERVER
//--------------------------------------------------------------------------------------------------
/**
 * Connection attempt has timeout
 */
//--------------------------------------------------------------------------------------------------
static void ConnectionTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< [IN] This timer has expired
)
{
    if (SerialHandleRecord.connState == LE_COMM_CONNECTING)
    {
        // time out the connection
        LE_ASSERT(AsyncConnectionHandlerFuncPtr != NULL);
        AsyncConnectionHandlerFuncPtr(&SerialHandleRecord, POLLERR);
        le_timer_Delete(timerRef);
    }
    else
    {
        le_timer_Delete(timerRef);
    }
}
#else
static void ConnectionTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< [IN] This timer has expired
)
{
    static int count = 0;
    if (SerialHandleRecord.connState == LE_COMM_CONNECTING)
    {
        // For the client side, the timer duration is always 1 second and the time out is used
        // as a retry maximum.
        if (count == LE_COMM_CONNECTION_TIMEOUT_TIMER_DURATION)
        {
            // time out the connection
            LE_ASSERT(AsyncConnectionHandlerFuncPtr != NULL);
            AsyncConnectionHandlerFuncPtr(&SerialHandleRecord, POLLERR);
            le_timer_Delete(timerRef);
            count = 0;
        }
        else
        {
            // try again.
            count++;
            // send the LE_COMM_CONNECT_HELLO signal.
            const char helloChar = LE_COMM_CONNECT_HELLO;
            LE_ASSERT(le_fd_Write(SerialHandleRecord.serialFd, &helloChar, 1) == 1);
             // start the time again:
            le_timer_Start(timerRef);
        }
    }
    else
    {
        count = 0;
        le_timer_Delete(timerRef);
    }
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Start a timer to recent the hello character
 */
//--------------------------------------------------------------------------------------------------
static void StartConnectionTimeoutTimer(void)
{
    le_timer_Ref_t  connectionTimerRef;
#ifdef LE_COMM_SERVER
    le_clk_Time_t timerInterval = { .sec=LE_COMM_CONNECTION_TIMEOUT_TIMER_DURATION, .usec=0 };
#else
    le_clk_Time_t timerInterval = { .sec=1, .usec=0 };
#endif
    // Create a timer to trigger a Network status check
    connectionTimerRef = le_timer_Create("le_comm_connect");
    le_timer_SetInterval(connectionTimerRef, timerInterval);
    le_timer_SetHandler(connectionTimerRef, ConnectionTimerExpiryHandler);
    le_timer_SetWakeup(connectionTimerRef, false);
    // Start timer
    le_timer_Start(connectionTimerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Callback registered to fd monitor that gets called whenever there's an event on the fd
 */
//--------------------------------------------------------------------------------------------------
static void AsyncRecvHandler
(
    int handle,             ///< [IN] the fd that this callback is being called for
    short events            ///< [IN] events that has happened on this fd
)
{
    char tmp;

    LE_DEBUG("Handle provided to fd monitor got called");
    if (handle != SerialHandleRecord.serialFd)
    {
        LE_ERROR("Unable to find matching Handle Record, fd [%" PRIiPTR "]", (intptr_t) handle);
        return;
    }
    if (SerialHandleRecord.connState == LE_COMM_CONNECTING)
    {
        char response = 0;
        if(!le_fd_Read(handle, &response, 1))
        {
            // nothing to read
            return;
        }
        LE_ASSERT(AsyncConnectionHandlerFuncPtr != NULL);
#ifdef LE_COMM_SERVER
        if (response == LE_COMM_CONNECT_HELLO)
        {
            // received a hello from a client. we're connected then.
            SerialHandleRecord.connState = LE_COMM_CONNECTED;
            const char helloAckChar = LE_COMM_CONNECT_HELLOACK;
            LE_ASSERT(le_fd_Write(handle, &helloAckChar, 1) == 1);
            AsyncConnectionHandlerFuncPtr(&SerialHandleRecord, events);
        }
        else
        {
            // Discard any data that is in the receive buffer for this fd
            // before returning
            goto discard;
        }
#else
        if (response == LE_COMM_CONNECT_HELLOACK)
        {
            // received an ack to our hello. we're now officially connected.
            SerialHandleRecord.connState = LE_COMM_CONNECTED;
            AsyncConnectionHandlerFuncPtr(&SerialHandleRecord, events);
            if (AsyncReceiveHandlerFuncPtr != NULL)
            {
                // Call the RPC Proxy's data receive handler in the event
                // new data has been received over the serial link
                // during this time
                AsyncReceiveHandlerFuncPtr(&SerialHandleRecord, events);
            }
        }
#endif
        return;
    }
    else if (SerialHandleRecord.connState == LE_COMM_CONNECTED)
    {
        // Notify the RPC Proxy
        if (AsyncReceiveHandlerFuncPtr != NULL)
        {
            AsyncReceiveHandlerFuncPtr(&SerialHandleRecord, events);
        }
        else
        {
            LE_ERROR("rpcProxy has not registered a call for for receiving events");
        }

        return;
    }
    else
    {
        // Discard any data that is in the receive buffer for this fd
        // before returning
        goto discard;
    }

discard:
    //purge the whatever is on the fd
    // read until there's nothing left.
    while (le_fd_Read(handle, &tmp, 1) >= 1)
    {
        // Discard data
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to Parse Command Line Arguments
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseCommandLineArgs
(
    const int argc,
    const char* argv[]
)
{
    le_result_t result = LE_OK;

    LE_DEBUG("Parsing Command Line Arguments %d", argc);

    if (argc != 1)
    {
        LE_ERROR("Invalid Command Line Argument, argc = [%d]", argc);
        result = LE_BAD_PARAMETER;
    }
    else if (argc == 1 && argv[0] != NULL)
    {
        le_utf8_Copy(SerialHandleRecord.serialDeviceName, argv[0],
            sizeof(SerialHandleRecord.serialDeviceName), NULL);
        LE_INFO("Setting Internal UART device name [%s]", SerialHandleRecord.serialDeviceName);
    }
    else
    {
        LE_ERROR("Null argument provided for serial device name");
        result = LE_BAD_PARAMETER;
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for Creating a RPC Serial Communication Channel
 *
 * Return Code values:
 *      - LE_OK if successfully,
 *      - LE_IN_PROGRESS if pending on the other side to come up,
 *      - otherwise failure
 *
 * @return
 *      Opaque handle to the Communication Channel.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void* le_comm_Create
(
    const int argc,         ///< [IN] Number of strings pointed to by argv.
    const char *argv[],     ///< [IN] Pointer to an array of character strings.
    le_result_t* resultPtr  ///< [OUT] Return Code
)
{
    LE_DEBUG("le_comm_Create called with argc: %d",argc);
    // Verify result pointer is valid
    if (resultPtr == NULL)
    {
        LE_ERROR("resultPtr is NULL");
        return NULL;
    }

    // Parse the Command Line arguments to extract the IP Address and TCP Port number
    *resultPtr = ParseCommandLineArgs(argc, argv);
    if (*resultPtr != LE_OK)
    {
        return NULL;
    }

    SerialHandleRecord.serialFd = le_fd_Open(SerialHandleRecord.serialDeviceName, O_RDWR);
    if (SerialHandleRecord.serialFd == -1)
    {
        return NULL;
    }
    LE_INFO("Opened Serial device Fd:[%d]", SerialHandleRecord.serialFd);

    int opts = le_fd_Fcntl(SerialHandleRecord.serialFd, F_GETFL);
    if (opts < 0) {
        LE_ERROR("le_fd_Fcntl(F_GETFL)");
        *resultPtr = LE_FAULT;
        return NULL;
    }
    // Set Non-Blocking socket option
    opts |= O_NONBLOCK;
    if (le_fd_Fcntl(SerialHandleRecord.serialFd, F_SETFL, opts) < 0) {
        LE_ERROR("le_fd_Fcntl(F_SETFL)");
        *resultPtr = LE_FAULT;
        return NULL;
    }

    // Create thread to monitor FD handle for activity, as defined by the events
    SerialHandleRecord.fdMonitorRef = le_fdMonitor_Create("serialComm_FD",
                                        SerialHandleRecord.serialFd,
                                        (le_fdMonitor_HandlerFunc_t) &AsyncRecvHandler,
                                        POLLIN);

    return (&SerialHandleRecord);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for Registering a Callback Handler function to monitor events on the specific handle
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_comm_RegisterHandleMonitor
(
    void* handle,           ///< [IN] handle record that the callback is being registered for
    le_comm_CallbackHandlerFunc_t handlerFunc, ///< [IN] function pointer to register as callback
    short events            ///< [IN] The events to register the callback for
)
{
    LE_UNUSED(events);
    LE_ASSERT(handlerFunc);
    HandleRecordPtr_t recordPtr = (HandleRecordPtr_t)handle;
    LE_DEBUG("le_comm_RegisterHandleMonitor called for fd:[%d]", recordPtr->serialFd);
    if (events & POLLIN)
    {
        AsyncReceiveHandlerFuncPtr = handlerFunc;
    }
    else
    {
        AsyncConnectionHandlerFuncPtr = handlerFunc;
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for Deleting RPC Network-Socket Communication Channel
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_comm_Delete
(
    void* handle             ///< [IN] pointer to handle record to delete.
)
{
    HandleRecordPtr_t recordPtr = (HandleRecordPtr_t)handle;
    LE_DEBUG("le_comm_Delete called for fd:[%d]", recordPtr->serialFd);
    le_fdMonitor_Delete(recordPtr->fdMonitorRef);
    le_fd_Close(recordPtr->serialFd);
    recordPtr->serialFd = 0;
    recordPtr->connState = LE_COMM_IDLE;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Connecting to another le_comm. A two way handshake is used.
 *
 * The server side starts by listening on the port and waiting for an LE_COMM_CONNECT_HELLO
 * character. If anything other than LE_COMM_CONNECT_HELLO is received on the port it is discarded.
 * The client side starts by sending an LE_COMM_CONNECT_HELLO character and waiting for a response.
 * Once the server receives an LE_COMM_CONNECT_HELLO it will reply by sending a
 * LE_COMM_CONNECT_HELLOACK to the client and marks the connection as connected.
 * The client goes to the connected mode once it receives the LE_COMM_CONNECT_HELLOACK character.
 * This call is non blocking. One the connection is established AsyncConnectionHandlerFuncPtr will
 * be called.
 * @return
 *      - LE_IN_PROGRESS if the connection handshake has started.
 *      - LE_OK if already connected.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_comm_Connect
(
    void* handle             ///< [IN] pointer to handle record to connect.
)
{
    // Check whether or not the other side is open:
    HandleRecordPtr_t recordPtr = (HandleRecordPtr_t)handle;
    LE_DEBUG("le_comm_Connect called for fd:[%d]",recordPtr->serialFd);
    if (SerialHandleRecord.connState == LE_COMM_CONNECTED)
    {
        return LE_OK;
    }
    recordPtr->connState = LE_COMM_CONNECTING;
#ifndef LE_COMM_SERVER
    // send the LE_COMM_CONNECT_HELLO signal.
    const char helloChar = LE_COMM_CONNECT_HELLO;
    LE_ASSERT(le_fd_Write(recordPtr->serialFd, &helloChar, 1) == 1);
#endif
    // start a timeout timer:
    StartConnectionTimeoutTimer();
    return LE_IN_PROGRESS;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to disconnect the le_comm session
 *
 * @return
 *      - LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_comm_Disconnect
(
    void* handle             ///< [IN] pointer to handle record to disconnect.
)
{
    HandleRecordPtr_t recordPtr = (HandleRecordPtr_t)handle;
    LE_DEBUG("le_comm_Disconnect called for fd:[%d]", recordPtr->serialFd);
    recordPtr->connState = LE_COMM_IDLE;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for rpc proxy to call in order to send data
 *
 * @return
 *      - LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_comm_Send
(
    void* handle,            ///< [IN] pointer to handle record to send on
    const void* buf,         ///< [IN] pointer to buffer holding data to send.
    size_t len               ///< [IN] number of bytes to send.
)
{
    HandleRecordPtr_t recordPtr = (HandleRecordPtr_t)handle;
    LE_DEBUG("le_comm_Send called to send %d bytes to fd:[%d]", len, recordPtr->serialFd);
    LE_DUMP(buf, len);
    LE_ASSERT(recordPtr->connState == LE_COMM_CONNECTED);
    LE_ASSERT(le_fd_Write(recordPtr->serialFd, buf, len) == (int)len);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for rpc proxy to call when it wants to receive data
 *
 * @return
 *      - LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_comm_Receive
(
    void* handle,            ///< [IN] pointer to handle record to receive on
    void* buf,               ///< [OUT] pointer to buffer to store received data
    size_t* len              ///< [IN/OUT] pointer to size of buffer and then number of bytes read
)
{
    HandleRecordPtr_t recordPtr = (HandleRecordPtr_t)handle;
    LE_DEBUG("le_comm_Receive called to get %d bytes from fd:[%d]", *len, recordPtr->serialFd);
    LE_ASSERT(recordPtr->connState == LE_COMM_CONNECTED);
    *len = le_fd_Read(recordPtr->serialFd, buf, *len);
    LE_DUMP(buf, *len);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve an ID for the specified handle.
 * NOTE:  For logging or display purposes only.
 *
 * @return
 *      - Non-zero integer, if successful.
 *      - Negative one (-1), otherwise.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int le_comm_GetId
(
    void* handle             ///< [IN] pointer to handle record to get ID
)
{
    HandleRecordPtr_t recordPtr = (HandleRecordPtr_t)handle;
    if (handle == NULL)
    {
        return -1;
    }

    LE_DEBUG("le_comm_GetId called for fd:[%d]", recordPtr->serialFd);

    return recordPtr->serialFd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for returning the parent handle
 *
 * @return
 *      - same handle because the there's no parent handle with serial fds.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void* le_comm_GetParentHandle
(
    void* handle             ///< [IN] pointer to handle record to get parent
)
{
    HandleRecordPtr_t recordPtr = (HandleRecordPtr_t)handle;
    LE_DEBUG("RPC: le_comm_GetParentHandle called for fd:[%d]", recordPtr->serialFd);
    return (&SerialHandleRecord);
}
