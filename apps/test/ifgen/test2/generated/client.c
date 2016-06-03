/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#include "messages.h"
#include "interface.h"


//--------------------------------------------------------------------------------------------------
// Generic Pack/Unpack Functions
//--------------------------------------------------------------------------------------------------

// todo: These functions could be moved to a separate library, to reduce overall code size and RAM
//       usage because they are common to each client and server.  However, they would then likely
//       need to be more generic, and provide better parameter checking and return results.  With
//       the way they are now, they can be customized to the specific needs of the generated code,
//       so for now, they will be kept with the generated code.  This may need revisiting later.

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* PackData(void* msgBufPtr, const void* dataPtr, size_t dataSize)
{
    // todo: should check for buffer overflow, but not sure what to do if it happens
    //       i.e. is it a fatal error, or just return a result

    memcpy( msgBufPtr, dataPtr, dataSize );
    return ( msgBufPtr + dataSize );
}

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* UnpackData(void* msgBufPtr, void* dataPtr, size_t dataSize)
{
    memcpy( dataPtr, msgBufPtr, dataSize );
    return ( msgBufPtr + dataSize );
}

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* PackString(void* msgBufPtr, const char* dataStr)
{
    // todo: should check for buffer overflow, but not sure what to do if it happens
    //       i.e. is it a fatal error, or just return a result

    // Get the sizes
    uint32_t strSize = strlen(dataStr);
    const uint32_t sizeOfStrSize = sizeof(strSize);

    // Always pack the string size first, and then the string itself
    memcpy( msgBufPtr, &strSize, sizeOfStrSize );
    msgBufPtr += sizeOfStrSize;
    memcpy( msgBufPtr, dataStr, strSize );

    // Return pointer to next free byte; msgBufPtr was adjusted above for string size value.
    return ( msgBufPtr + strSize );
}

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* UnpackString(void* msgBufPtr, char* dataStr, size_t dataSize)
{
    // todo: should check for buffer overflow, but not sure what to do if it happens
    //       i.e. is it a fatal error, or just return a result

    uint32_t strSize;
    const uint32_t sizeOfStrSize = sizeof(strSize);

    // Get the string size first, and then the actual string
    memcpy( &strSize, msgBufPtr, sizeOfStrSize );
    msgBufPtr += sizeOfStrSize;

    // Copy the string, and make sure it is null-terminated
    memcpy( dataStr, msgBufPtr, strSize );
    dataStr[strSize] = 0;

    // Return pointer to next free byte; msgBufPtr was adjusted above for string size value.
    return ( msgBufPtr + strSize );
}


//--------------------------------------------------------------------------------------------------
// Generic Client Types, Variables and Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Client Data Objects
 *
 * This object is used for each registered handler.  This is needed since we are not using
 * events, but are instead queueing functions directly with the event loop.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_HandlerFunc_t handlerPtr;          ///< Registered handler function
    void*                  contextPtr;          ///< ContextPtr registered with handler
    le_event_HandlerRef_t  handlerRef;          ///< HandlerRef for the registered handler
    le_thread_Ref_t        callersThreadRef;    ///< Caller's thread.
}
_ClientData_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for client data objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t _ClientDataPool;


//--------------------------------------------------------------------------------------------------
/**
 * Client Thread Objects
 *
 * This object is used to contain thread specific data for each IPC client.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t sessionRef;     ///< Client Session Reference
    int                 clientCount;    ///< Number of clients sharing this thread
}
_ClientThreadData_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for client thread objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t _ClientThreadDataPool;


//--------------------------------------------------------------------------------------------------
/**
 * Key under which the pointer to the Thread Object (_ClientThreadData_t) will be kept in
 * thread-local storage.  This allows a thread to quickly get a pointer to its own Thread Object.
 */
//--------------------------------------------------------------------------------------------------
static pthread_key_t _ThreadDataKey;


//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for use with Add/Remove handler references
 *
 * @warning Use _Mutex, defined below, to protect accesses to this data.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t _HandlerRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Mutex and associated macros for use with the above HandlerRefMap.
 *
 * Unused attribute is needed because this variable may not always get used.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static pthread_mutex_t _Mutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.

/// Locks the mutex.
#define _LOCK    LE_ASSERT(pthread_mutex_lock(&_Mutex) == 0);

/// Unlocks the mutex.
#define _UNLOCK  LE_ASSERT(pthread_mutex_unlock(&_Mutex) == 0);


//--------------------------------------------------------------------------------------------------
/**
 * This global flag is shared by all client threads, and is used to indicate whether the common
 * data has been initialized.
 *
 * @warning Use InitMutex, defined below, to protect accesses to this data.
 */
//--------------------------------------------------------------------------------------------------
static bool CommonDataInitialized = false;


//--------------------------------------------------------------------------------------------------
/**
 * Mutex and associated macros for use with the above CommonDataInitialized.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t InitMutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.

/// Locks the mutex.
#define LOCK_INIT    LE_ASSERT(pthread_mutex_lock(&InitMutex) == 0);

/// Unlocks the mutex.
#define UNLOCK_INIT  LE_ASSERT(pthread_mutex_unlock(&InitMutex) == 0);


//--------------------------------------------------------------------------------------------------
/**
 * Forward declaration needed by InitClientForThread
 */
//--------------------------------------------------------------------------------------------------
static void ClientIndicationRecvHandler
(
    le_msg_MessageRef_t  msgRef,
    void*                contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize thread specific data, and connect to the service for the current thread.
 *
 * @return
 *  - LE_OK if the client connected successfully to the service.
 *  - LE_UNAVAILABLE if the server is not currently offering the service to which the client is bound.
 *  - LE_NOT_PERMITTED if the client interface is not bound to any service (doesn't have a binding).
 *  - LE_COMM_ERROR if the Service Directory cannot be reached.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InitClientForThread
(
    bool isBlocking
)
{
    // Open a session.
    le_msg_ProtocolRef_t protocolRef;
    le_msg_SessionRef_t sessionRef;

    protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID_STR, sizeof(_Message_t));
    sessionRef = le_msg_CreateSession(protocolRef, SERVICE_INSTANCE_NAME);
    le_msg_SetSessionRecvHandler(sessionRef, ClientIndicationRecvHandler, NULL);

    if ( isBlocking )
    {
        le_msg_OpenSessionSync(sessionRef);
    }
    else
    {
        le_result_t result;

        result = le_msg_TryOpenSessionSync(sessionRef);
        if ( result != LE_OK )
        {
            LE_DEBUG("Could not connect to '%s' service", SERVICE_INSTANCE_NAME);

            // TODO: This should be le_msg_DeleteSession(), but there's a bug in it.
            le_msg_CloseSession(sessionRef);

            switch (result)
            {
                case LE_UNAVAILABLE:
                    LE_DEBUG("Service not offered");
                    break;

                case LE_NOT_PERMITTED:
                    LE_DEBUG("Missing binding");
                    break;

                case LE_COMM_ERROR:
                    LE_DEBUG("Can't reach ServiceDirectory");
                    break;

                default:
                    LE_CRIT("le_msg_TryOpenSessionSync() returned unexpected result code %d (%s)",
                            result,
                            LE_RESULT_TXT(result));
                    break;
            }

            return result;
        }
    }

    // Store the client sessionRef in thread-local storage, since each thread requires
    // its own sessionRef.
    _ClientThreadData_t* clientThreadPtr = le_mem_ForceAlloc(_ClientThreadDataPool);
    clientThreadPtr->sessionRef = sessionRef;
    if (pthread_setspecific(_ThreadDataKey, clientThreadPtr) != 0)
    {
        LE_FATAL("pthread_setspecific() failed!");
    }

    // This is the first client for the current thread
    clientThreadPtr->clientCount = 1;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a pointer to the client thread data for the current thread.
 *
 * If the current thread does not have client data, then NULL is returned
 */
//--------------------------------------------------------------------------------------------------
static _ClientThreadData_t* GetClientThreadDataPtr
(
    void
)
{
    return pthread_getspecific(_ThreadDataKey);
}


//--------------------------------------------------------------------------------------------------
/**
 * Return the sessionRef for the current thread.
 *
 * If the current thread does not have a session ref, then this is a fatal error.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static le_msg_SessionRef_t GetCurrentSessionRef
(
    void
)
{
    _ClientThreadData_t* clientThreadPtr = GetClientThreadDataPtr();

    // If the thread specific data is NULL, then the session ref has not been created.
    LE_FATAL_IF(clientThreadPtr==NULL,
                "ConnectService() not called for current thread");

    return clientThreadPtr->sessionRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Init data that is common across all threads.
 */
//--------------------------------------------------------------------------------------------------
static void InitCommonData(void)
{
    // Allocate the client data pool
    _ClientDataPool = le_mem_CreatePool("ClientData", sizeof(_ClientData_t));

    // Allocate the client thread pool
    _ClientThreadDataPool = le_mem_CreatePool("ClientThreadData", sizeof(_ClientThreadData_t));

    // Create the thread-local data key to be used to store a pointer to each thread object.
    LE_ASSERT(pthread_key_create(&_ThreadDataKey, NULL) == 0);

    // Create safe reference map for handler references.
    // The size of the map should be based on the number of handlers defined multiplied by
    // the number of client threads.  Since this number can't be completely determined at
    // build time, just make a reasonable guess.
    _HandlerRefMap = le_ref_CreateMap("ClientHandlers", 5);
}


//--------------------------------------------------------------------------------------------------
/**
 * Connect to the service, using either blocking or non-blocking calls.
 *
 * This function implements the details of the public ConnectService functions.
 *
 * @return
 *  - LE_OK if the client connected successfully to the service.
 *  - LE_UNAVAILABLE if the server is not currently offering the service to which the client is bound.
 *  - LE_NOT_PERMITTED if the client interface is not bound to any service (doesn't have a binding).
 *  - LE_COMM_ERROR if the Service Directory cannot be reached.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DoConnectService
(
    bool isBlocking
)
{
    // If this is the first time the function is called, init the client common data.
    LOCK_INIT
    if ( ! CommonDataInitialized )
    {
        InitCommonData();
        CommonDataInitialized = true;
    }
    UNLOCK_INIT

    _ClientThreadData_t* clientThreadPtr = GetClientThreadDataPtr();

    // If the thread specific data is NULL, then there is no current client session.
    if (clientThreadPtr == NULL)
    {
        le_result_t result;

        result = InitClientForThread(isBlocking);
        if ( result != LE_OK )
        {
            // Note that the blocking call will always return LE_OK
            return result;
        }

        LE_DEBUG("======= Starting client for '%s' service ========", SERVICE_INSTANCE_NAME);
    }
    else
    {
        // Keep track of the number of clients for the current thread.  There is only one
        // connection per thread, and it is shared by all clients.
        clientThreadPtr->clientCount++;
        LE_DEBUG("======= Starting another client for '%s' service ========", SERVICE_INSTANCE_NAME);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 *
 * Connect the current client thread to the service providing this API. Block until the service is
 * available.
 *
 * For each thread that wants to use this API, either ConnectService or TryConnectService must be
 * called before any other functions in this API.  Normally, ConnectService is automatically called
 * for the main thread, but not for any other thread. For details, see @ref apiFilesC_client.
 *
 * This function is created automatically.
 */
//--------------------------------------------------------------------------------------------------
void ConnectService
(
    void
)
{
    // Conect to the service; block until connected.
    DoConnectService(true);
}

//--------------------------------------------------------------------------------------------------
/**
 *
 * Try to connect the current client thread to the service providing this API. Return with an error
 * if the service is not available.
 *
 * For each thread that wants to use this API, either ConnectService or TryConnectService must be
 * called before any other functions in this API.  Normally, ConnectService is automatically called
 * for the main thread, but not for any other thread. For details, see @ref apiFilesC_client.
 *
 * This function is created automatically.
 *
 * @return
 *  - LE_OK if the client connected successfully to the service.
 *  - LE_UNAVAILABLE if the server is not currently offering the service to which the client is bound.
 *  - LE_NOT_PERMITTED if the client interface is not bound to any service (doesn't have a binding).
 *  - LE_COMM_ERROR if the Service Directory cannot be reached.
 */
//--------------------------------------------------------------------------------------------------
le_result_t TryConnectService
(
    void
)
{
    // Conect to the service; return with an error if not connected.
    return DoConnectService(false);
}

//--------------------------------------------------------------------------------------------------
/**
 *
 * Disconnect the current client thread from the service providing this API.
 *
 * Normally, this function doesn't need to be called. After this function is called, there's no
 * longer a connection to the service, and the functions in this API can't be used. For details, see
 * @ref apiFilesC_client.
 *
 * This function is created automatically.
 */
//--------------------------------------------------------------------------------------------------
void DisconnectService
(
    void
)
{
    _ClientThreadData_t* clientThreadPtr = GetClientThreadDataPtr();

    // If the thread specific data is NULL, then there is no current client session.
    if (clientThreadPtr == NULL)
    {
        LE_CRIT("Trying to stop non-existent client session for '%s' service",
                SERVICE_INSTANCE_NAME);
    }
    else
    {
        // This is the last client for this thread, so close the session.
        if ( clientThreadPtr->clientCount == 1 )
        {
            // TODO: This should be le_msg_DeleteSession(), but there's a bug in it.
            le_msg_CloseSession( clientThreadPtr->sessionRef );

            // Need to delete the thread specific data, since it is no longer valid.  If a new
            // client session is started, new thread specific data will be allocated.
            le_mem_Release(clientThreadPtr);
            if (pthread_setspecific(_ThreadDataKey, NULL) != 0)
            {
                LE_FATAL("pthread_setspecific() failed!");
            }

            LE_DEBUG("======= Stopping client for '%s' service ========", SERVICE_INSTANCE_NAME);
        }
        else
        {
            // There is one less client sharing this thread's connection.
            clientThreadPtr->clientCount--;

            LE_DEBUG("======= Stopping another client for '%s' service ========",
                     SERVICE_INSTANCE_NAME);
        }
    }
}


//--------------------------------------------------------------------------------------------------
// Client Specific Client Code
//--------------------------------------------------------------------------------------------------


// This function parses the message buffer received from the server, and then calls the user
// registered handler, which is stored in a client data object.
static void _Handle_AddTestAHandler
(
    void* _reportPtr,
    void* _dataPtr
)
{
    le_msg_MessageRef_t _msgRef = _reportPtr;
    _Message_t* _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    uint8_t* _msgBufPtr = _msgPtr->buffer;

    // The clientContextPtr always exists and is always first. It is a safe reference to the client
    // data object, but we already get the pointer to the client data object through the _dataPtr
    // parameter, so we don't need to do anything with clientContextPtr, other than unpacking it.
    void* _clientContextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &_clientContextPtr, sizeof(void*) );

    // The client data pointer is passed in as a parameter, since the lookup in the safe ref map
    // and check for NULL has already been done when this function is queued.
    _ClientData_t* _clientDataPtr = _dataPtr;

    // Pull out additional data from the client data pointer
    TestAHandlerFunc_t _handlerRef_AddTestAHandler = (TestAHandlerFunc_t)_clientDataPtr->handlerPtr;
    void* contextPtr = _clientDataPtr->contextPtr;

    // Unpack the remaining parameters
    int32_t x;
    _msgBufPtr = UnpackData( _msgBufPtr, &x, sizeof(int32_t) );


    // Call the registered handler
    if ( _handlerRef_AddTestAHandler != NULL )
    {
        _handlerRef_AddTestAHandler( x, contextPtr );
    }
    else
    {
        LE_FATAL("Error in client data: no registered handler");
    }


    // Release the message, now that we are finished with it.
    le_msg_ReleaseMsg(_msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'TestA'
 *
 * This event is used for testing EVENTS and Add/Remove handler functions
 */
//--------------------------------------------------------------------------------------------------
TestAHandlerRef_t AddTestAHandler
(
    TestAHandlerFunc_t myHandlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    TestAHandlerRef_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_AddTestAHandler;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    // The handlerPtr and contextPtr input parameters are stored in the client data object, and it is
    // a safe reference to this object that is passed down as the context pointer.  The handlerPtr is
    // not passed down.
    // Create a new client data object and fill it in
    _ClientData_t* _clientDataPtr = le_mem_ForceAlloc(_ClientDataPool);
    _clientDataPtr->handlerPtr = (le_event_HandlerFunc_t)myHandlerPtr;
    _clientDataPtr->contextPtr = contextPtr;
    _clientDataPtr->callersThreadRef = le_thread_GetCurrent();
    // Create a safeRef to be passed down as the contextPtr
    _LOCK
    contextPtr = le_ref_CreateRef(_HandlerRefMap, _clientDataPtr);
    _UNLOCK
    _msgBufPtr = PackData( _msgBufPtr, &contextPtr, sizeof(void*) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );
    // Put the handler reference result into the client data object, and
    // then return a safe reference to the client data object as the reference;
    // this safe reference is contained in the contextPtr, which was assigned
    // when the client data object was created.
    _clientDataPtr->handlerRef = (le_event_HandlerRef_t)_result;
    _result = contextPtr;

    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);

    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'TestA'
 */
//--------------------------------------------------------------------------------------------------
void RemoveTestAHandler
(
    TestAHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;



    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_RemoveTestAHandler;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    // The passed in handlerRef is a safe reference for the client data object.  Need to get the
    // real handlerRef from the client data object and then delete both the safe reference and
    // the object since they are no longer needed.
    _LOCK
    _ClientData_t* clientDataPtr = le_ref_Lookup(_HandlerRefMap, addHandlerRef);
    LE_FATAL_IF(clientDataPtr==NULL, "Invalid reference");
    le_ref_DeleteRef(_HandlerRefMap, addHandlerRef);
    _UNLOCK
    addHandlerRef = (TestAHandlerRef_t)clientDataPtr->handlerRef;
    le_mem_Release(clientDataPtr);
    _msgBufPtr = PackData( _msgBufPtr, &addHandlerRef, sizeof(TestAHandlerRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;


    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function takes all the possible kinds of parameters, but returns nothing
 */
//--------------------------------------------------------------------------------------------------
void allParameters
(
    common_EnumExample_t a,
        ///< [IN] first one-line comment
        ///<      second one-line comment

    uint32_t* bPtr,
        ///< [OUT]

    const uint32_t* dataPtr,
        ///< [IN]

    size_t dataNumElements,
        ///< [IN]

    uint32_t* outputPtr,
        ///< [OUT] some more comments here
        ///<       and some comments here as well

    size_t* outputNumElementsPtr,
        ///< [INOUT]

    const char* label,
        ///< [IN]

    char* response,
        ///< [OUT] comments on final parameter, first line
        ///<       and more comments

    size_t responseNumElements,
        ///< [IN]

    char* more,
        ///< [OUT] This parameter tests a bug fix

    size_t moreNumElements
        ///< [IN]
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;



    // Range check values, if appropriate
    if ( dataNumElements > 10 ) LE_FATAL("dataNumElements > 10");
    if ( strlen(label) > 20 ) LE_FATAL("strlen(label) > 20");


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_allParameters;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &a, sizeof(common_EnumExample_t) );
    _msgBufPtr = PackData( _msgBufPtr, &dataNumElements, sizeof(size_t) );
    _msgBufPtr = PackData( _msgBufPtr, dataPtr, dataNumElements*sizeof(uint32_t) );
    _msgBufPtr = PackData( _msgBufPtr, outputNumElementsPtr, sizeof(size_t) );
    _msgBufPtr = PackString( _msgBufPtr, label );
    _msgBufPtr = PackData( _msgBufPtr, &responseNumElements, sizeof(size_t) );
    _msgBufPtr = PackData( _msgBufPtr, &moreNumElements, sizeof(size_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, bPtr, sizeof(uint32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, outputNumElementsPtr, sizeof(size_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, outputPtr, *outputNumElementsPtr*sizeof(uint32_t) );
    _msgBufPtr = UnpackString( _msgBufPtr, response, responseNumElements*sizeof(char) );
    _msgBufPtr = UnpackString( _msgBufPtr, more, moreNumElements*sizeof(char) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test file descriptors as IN and OUT parameters
 */
//--------------------------------------------------------------------------------------------------
void FileTest
(
    int dataFile,
        ///< [IN] file descriptor as IN parameter

    int* dataOutPtr
        ///< [OUT] file descriptor as OUT parameter
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;



    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_FileTest;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    le_msg_SetFd(_msgRef, dataFile);

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;


    // Unpack any "out" parameters
    *dataOutPtr = le_msg_GetFd(_responseMsgRef);

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function fakes an event, so that the handler will be called.
 * Only needed for testing.  Would never exist on a real system.
 */
//--------------------------------------------------------------------------------------------------
void TriggerTestA
(
    void
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;



    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_TriggerTestA;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters


    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;


    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);
}


// This function parses the message buffer received from the server, and then calls the user
// registered handler, which is stored in a client data object.
static void _Handle_AddBugTestHandler
(
    void* _reportPtr,
    void* _dataPtr
)
{
    le_msg_MessageRef_t _msgRef = _reportPtr;
    _Message_t* _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    uint8_t* _msgBufPtr = _msgPtr->buffer;

    // The clientContextPtr always exists and is always first. It is a safe reference to the client
    // data object, but we already get the pointer to the client data object through the _dataPtr
    // parameter, so we don't need to do anything with clientContextPtr, other than unpacking it.
    void* _clientContextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &_clientContextPtr, sizeof(void*) );

    // The client data pointer is passed in as a parameter, since the lookup in the safe ref map
    // and check for NULL has already been done when this function is queued.
    _ClientData_t* _clientDataPtr = _dataPtr;

    // Pull out additional data from the client data pointer
    BugTestHandlerFunc_t _handlerRef_AddBugTestHandler = (BugTestHandlerFunc_t)_clientDataPtr->handlerPtr;
    void* contextPtr = _clientDataPtr->contextPtr;

    // Unpack the remaining parameters


    // Call the registered handler
    if ( _handlerRef_AddBugTestHandler != NULL )
    {
        _handlerRef_AddBugTestHandler( contextPtr );
    }
    else
    {
        LE_FATAL("Error in client data: no registered handler");
    }


    // Release the message, now that we are finished with it.
    le_msg_ReleaseMsg(_msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'BugTest'
 *
 * This event
 * is used for
 *     testing
 * a specific bug, as well as event comment strings.
 *
 * Uses old-style handler, for backwards compatibility testing
 */
//--------------------------------------------------------------------------------------------------
BugTestHandlerRef_t AddBugTestHandler
(
    const char* newPathPtr,
        ///< [IN]

    BugTestHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    BugTestHandlerRef_t _result;

    // Range check values, if appropriate
    if ( strlen(newPathPtr) > 512 ) LE_FATAL("strlen(newPathPtr) > 512");


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_AddBugTestHandler;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackString( _msgBufPtr, newPathPtr );
    // The handlerPtr and contextPtr input parameters are stored in the client data object, and it is
    // a safe reference to this object that is passed down as the context pointer.  The handlerPtr is
    // not passed down.
    // Create a new client data object and fill it in
    _ClientData_t* _clientDataPtr = le_mem_ForceAlloc(_ClientDataPool);
    _clientDataPtr->handlerPtr = (le_event_HandlerFunc_t)handlerPtr;
    _clientDataPtr->contextPtr = contextPtr;
    _clientDataPtr->callersThreadRef = le_thread_GetCurrent();
    // Create a safeRef to be passed down as the contextPtr
    _LOCK
    contextPtr = le_ref_CreateRef(_HandlerRefMap, _clientDataPtr);
    _UNLOCK
    _msgBufPtr = PackData( _msgBufPtr, &contextPtr, sizeof(void*) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );
    // Put the handler reference result into the client data object, and
    // then return a safe reference to the client data object as the reference;
    // this safe reference is contained in the contextPtr, which was assigned
    // when the client data object was created.
    _clientDataPtr->handlerRef = (le_event_HandlerRef_t)_result;
    _result = contextPtr;

    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);

    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'BugTest'
 */
//--------------------------------------------------------------------------------------------------
void RemoveBugTestHandler
(
    BugTestHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;



    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_RemoveBugTestHandler;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    // The passed in handlerRef is a safe reference for the client data object.  Need to get the
    // real handlerRef from the client data object and then delete both the safe reference and
    // the object since they are no longer needed.
    _LOCK
    _ClientData_t* clientDataPtr = le_ref_Lookup(_HandlerRefMap, addHandlerRef);
    LE_FATAL_IF(clientDataPtr==NULL, "Invalid reference");
    le_ref_DeleteRef(_HandlerRefMap, addHandlerRef);
    _UNLOCK
    addHandlerRef = (BugTestHandlerRef_t)clientDataPtr->handlerRef;
    le_mem_Release(clientDataPtr);
    _msgBufPtr = PackData( _msgBufPtr, &addHandlerRef, sizeof(BugTestHandlerRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;


    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);
}


// This function parses the message buffer received from the server, and then calls the user
// registered handler, which is stored in a client data object.
static void _Handle_TestCallback
(
    void* _reportPtr,
    void* _dataPtr
)
{
    le_msg_MessageRef_t _msgRef = _reportPtr;
    _Message_t* _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    uint8_t* _msgBufPtr = _msgPtr->buffer;

    // The clientContextPtr always exists and is always first. It is a safe reference to the client
    // data object, but we already get the pointer to the client data object through the _dataPtr
    // parameter, so we don't need to do anything with clientContextPtr, other than unpacking it.
    void* _clientContextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &_clientContextPtr, sizeof(void*) );

    // The client data pointer is passed in as a parameter, since the lookup in the safe ref map
    // and check for NULL has already been done when this function is queued.
    _ClientData_t* _clientDataPtr = _dataPtr;

    // Pull out additional data from the client data pointer
    CallbackTestHandlerFunc_t _handlerRef_TestCallback = (CallbackTestHandlerFunc_t)_clientDataPtr->handlerPtr;
    void* contextPtr = _clientDataPtr->contextPtr;

    // Unpack the remaining parameters
    uint32_t data;
    _msgBufPtr = UnpackData( _msgBufPtr, &data, sizeof(uint32_t) );

    char name[51];
    _msgBufPtr = UnpackString( _msgBufPtr, name, 51 );

    int dataFile;
    dataFile = le_msg_GetFd(_msgRef);


    // Call the registered handler
    if ( _handlerRef_TestCallback != NULL )
    {
        _handlerRef_TestCallback( data, name, dataFile, contextPtr );
    }
    else
    {
        LE_FATAL("Error in client data: no registered handler");
    }

    // The registered handler has been called, so no longer need the client data.
    // Explicitly set handlerPtr to NULL, so that we can catch if this function gets
    // accidently called again.
    _clientDataPtr->handlerPtr = NULL;
    le_mem_Release(_clientDataPtr);

    // Release the message, now that we are finished with it.
    le_msg_ReleaseMsg(_msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test function callback parameters
 */
//--------------------------------------------------------------------------------------------------
int32_t TestCallback
(
    uint32_t someParm,
        ///< [IN]

    const uint8_t* dataArrayPtr,
        ///< [IN]

    size_t dataArrayNumElements,
        ///< [IN]

    CallbackTestHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    int32_t _result;

    // Range check values, if appropriate
    if ( dataArrayNumElements > 5 ) LE_FATAL("dataArrayNumElements > 5");


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_TestCallback;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &someParm, sizeof(uint32_t) );
    _msgBufPtr = PackData( _msgBufPtr, &dataArrayNumElements, sizeof(size_t) );
    _msgBufPtr = PackData( _msgBufPtr, dataArrayPtr, dataArrayNumElements*sizeof(uint8_t) );
    // The handlerPtr and contextPtr input parameters are stored in the client data object, and it is
    // a safe reference to this object that is passed down as the context pointer.  The handlerPtr is
    // not passed down.
    // Create a new client data object and fill it in
    _ClientData_t* _clientDataPtr = le_mem_ForceAlloc(_ClientDataPool);
    _clientDataPtr->handlerPtr = (le_event_HandlerFunc_t)handlerPtr;
    _clientDataPtr->contextPtr = contextPtr;
    _clientDataPtr->callersThreadRef = le_thread_GetCurrent();
    // Create a safeRef to be passed down as the contextPtr
    _LOCK
    contextPtr = le_ref_CreateRef(_HandlerRefMap, _clientDataPtr);
    _UNLOCK
    _msgBufPtr = PackData( _msgBufPtr, &contextPtr, sizeof(void*) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );

    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);

    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to fake an event for the callback function testing
 */
//--------------------------------------------------------------------------------------------------
void TriggerCallbackTest
(
    uint32_t data
        ///< [IN]
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;



    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_TriggerCallbackTest;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &data, sizeof(uint32_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;


    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);
}


static void ClientIndicationRecvHandler
(
    le_msg_MessageRef_t  msgRef,
    void*                contextPtr
)
{
    // Get the message payload
    _Message_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
    uint8_t* _msgBufPtr = msgPtr->buffer;

    // Have to partially unpack the received message in order to know which thread
    // the queued function should actually go to.
    void* clientContextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &clientContextPtr, sizeof(void*) );

    // The clientContextPtr is a safe reference for the client data object.  If the client data
    // pointer is NULL, this means the handler was removed before the event was reported to the
    // client. This is valid, and the event will be dropped.
    _LOCK
    _ClientData_t* clientDataPtr = le_ref_Lookup(_HandlerRefMap, clientContextPtr);
    _UNLOCK
    if ( clientDataPtr == NULL )
    {
        LE_DEBUG("Ignore reported event after handler removed");
        return;
    }

    // Pull out the callers thread
    le_thread_Ref_t callersThreadRef = clientDataPtr->callersThreadRef;

    // Trigger the appropriate event
    switch (msgPtr->id)
    {
        case _MSGID_AddTestAHandler :
            le_event_QueueFunctionToThread(callersThreadRef, _Handle_AddTestAHandler, msgRef, clientDataPtr);
            break;

        case _MSGID_AddBugTestHandler :
            le_event_QueueFunctionToThread(callersThreadRef, _Handle_AddBugTestHandler, msgRef, clientDataPtr);
            break;

        case _MSGID_TestCallback :
            le_event_QueueFunctionToThread(callersThreadRef, _Handle_TestCallback, msgRef, clientDataPtr);
            break;

        default: LE_ERROR("Unknowm msg id = %i for client thread = %p", msgPtr->id, callersThreadRef);
    }
}

