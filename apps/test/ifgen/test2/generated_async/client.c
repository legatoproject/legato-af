/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#include "local.h"
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

    // Add one for the null character
    size_t dataSize = strlen(dataStr) + 1;

    memcpy( msgBufPtr, dataStr, dataSize );
    return ( msgBufPtr + dataSize );
}

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* UnpackString(void* msgBufPtr, const char** dataStrPtr)
{
    // Add one for the null character
    size_t dataSize = strlen(msgBufPtr) + 1;

    // Strings do not have to be word aligned, so just return a pointer
    // into the message buffer
    *dataStrPtr = msgBufPtr;
    return ( msgBufPtr + dataSize );
}

// todo: This function may eventually replace all usage of UnpackString() above.
//       Maybe there should also be a PackDataString() function as well?
// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* UnpackDataString(void* msgBufPtr, void* dataPtr, size_t dataSize)
{
    // Number of bytes copied from msg buffer, not including null terminator
    size_t numBytes;

    // todo: For now, assume the string will always fit in the buffer. This may not always be true.
    le_utf8_Copy( dataPtr, msgBufPtr, dataSize, &numBytes );
    return ( msgBufPtr + (numBytes + 1) );
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
    le_msg_SessionRef_t sessionRef;          ///< Client Session Reference
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
/* Maximum size of a service instance name string, including the null terminator byte.
 *
 * Based on LE_SVCDIR_MAX_SERVICE_NAME_SIZE in serviceDirectoryProtocol.h
 */
//--------------------------------------------------------------------------------------------------
#define MAX_SERVICE_NAME_SIZE    128


//--------------------------------------------------------------------------------------------------
/* The global service instance name is shared by all client threads.  It is only initialized once
 * by the main thread, and is only read by the other threads.  Thus, a mutex is not needed for
 * accesses to this variable.
 */
//--------------------------------------------------------------------------------------------------
static char GlobalServiceInstanceName[MAX_SERVICE_NAME_SIZE] = "";


//--------------------------------------------------------------------------------------------------
/**
 * Forward declaration needed by InitClientThreadData
 */
//--------------------------------------------------------------------------------------------------
static void ClientIndicationRecvHandler
(
    le_msg_MessageRef_t  msgRef,
    void*                contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Init thread specific data, and return a pointer to this data
 */
//--------------------------------------------------------------------------------------------------
static _ClientThreadData_t* InitClientThreadData
(
    const char* serviceInstanceName
)
{
    // Open a session.
    le_msg_ProtocolRef_t protocolRef;
    le_msg_SessionRef_t sessionRef;

    // The instance name must not be an empty string
    if ( serviceInstanceName[0] == '\0' )
    {
        LE_FATAL("Undefined client service instance name (Was StartClient() called?)");
    }

    protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID_STR, sizeof(_Message_t));
    sessionRef = le_msg_CreateSession(protocolRef, serviceInstanceName);
    le_msg_SetSessionRecvHandler(sessionRef, ClientIndicationRecvHandler, NULL);
    le_msg_OpenSessionSync(sessionRef);

    // Store the client sessionRef in thread-local storage, since each thread requires
    // its own sessionRef.
    _ClientThreadData_t* clientThreadPtr = le_mem_ForceAlloc(_ClientThreadDataPool);
    clientThreadPtr->sessionRef = sessionRef;
    if (pthread_setspecific(_ThreadDataKey, clientThreadPtr) != 0)
    {
        LE_FATAL("pthread_setspecific() failed!");
    }

    return clientThreadPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Return the sessionRef for the current thread.
 *
 * If the current thread does not have a session ref, then create it.
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t GetCurrentSessionRef
(
    void
)
{
    _ClientThreadData_t* clientThreadPtr = pthread_getspecific(_ThreadDataKey);

    // If the thread specific data is NULL, then the session ref has not been created yet.
    if (clientThreadPtr == NULL)
    {
        LE_DEBUG("======= Starting Client %s ========", GlobalServiceInstanceName);
        clientThreadPtr = InitClientThreadData(GlobalServiceInstanceName);
    }

    return clientThreadPtr->sessionRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Init data that is common across all threads.
 */
//--------------------------------------------------------------------------------------------------
static void InitClient(void)
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
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void StartClient
(
    const char* serviceInstanceName
        ///< [IN]
)
{
    // The instance name must not be an empty string
    if ( serviceInstanceName[0] == '\0' )
    {
        LE_FATAL("Service instance name is empty");
    }

    // If this is not the first time this function is called, compare against stored instance name.
    if ( GlobalServiceInstanceName[0] != '\0' )
    {
        if ( strcmp(GlobalServiceInstanceName, serviceInstanceName) == 0 )
        {
            LE_DEBUG("Called with duplicate name");
        }
        else
        {
            // This is an error because the user application is likely not connecting to the
            // service that they expect.
            LE_ERROR("Service instance name cannot be changed from '%s' to '%s'",
                     GlobalServiceInstanceName,
                     serviceInstanceName);
        }

        // Since the function was called before, there is nothing further to do.
        return;
    }

    // This is the first time the function is called.  Store the instance name and init the client.
    LE_FATAL_IF(le_utf8_Copy(GlobalServiceInstanceName,
                             serviceInstanceName,
                             sizeof(GlobalServiceInstanceName),
                             NULL) == LE_OVERFLOW,
                "Service ID '%s' too long (should only be %zu bytes total).",
                serviceInstanceName,
                sizeof(GlobalServiceInstanceName));

    LE_DEBUG("======= Starting Client %s ========", serviceInstanceName);
    InitClient();

    // Although InitClientThreadData() returns a value, it is not needed here.
    InitClientThreadData(serviceInstanceName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void StopClient
(
    void
)
{
    _ClientThreadData_t* clientThreadPtr = pthread_getspecific(_ThreadDataKey);

    // If the thread specific data is NULL, then there is no current client session.
    if (clientThreadPtr == NULL)
    {
        LE_ERROR("Trying to stop non-existent client session for '%s' service",
                 GlobalServiceInstanceName);
    }
    else
    {
        le_msg_CloseSession( clientThreadPtr->sessionRef );

        // Need to delete the thread specific data, since it is no longer valid.  If a new
        // client session is started, new thread specific data will be allocated.
        le_mem_Release(clientThreadPtr);
        if (pthread_setspecific(_ThreadDataKey, NULL) != 0)
        {
            LE_FATAL("pthread_setspecific() failed!");
        }

        LE_DEBUG("======= Stopping Client %s ========", GlobalServiceInstanceName);
    }
}


//--------------------------------------------------------------------------------------------------
// Client Specific Client Code
//--------------------------------------------------------------------------------------------------


// This function parses the message buffer received from the server, and then calls the user
// registered handler, which is stored in a client data object.
static void _Handle_AddTestA
(
    void* _reportPtr,
    void* _notUsed
)
{
    le_msg_MessageRef_t _msgRef = _reportPtr;
    _Message_t* _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    uint8_t* _msgBufPtr = _msgPtr->buffer;

    // The clientContextPtr always exists and is always first.
    void* _clientContextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &_clientContextPtr, sizeof(void*) );

    // Pull out additional data from the context pointer
    _ClientData_t* _clientDataPtr = _clientContextPtr;
    TestAFunc_t _handlerRef_AddTestA = (TestAFunc_t)_clientDataPtr->handlerPtr;
    void* contextPtr = _clientDataPtr->contextPtr;

    // Unpack the remaining parameters
    int32_t x;
    _msgBufPtr = UnpackData( _msgBufPtr, &x, sizeof(int32_t) );


    // Call the registered handler
    if ( _handlerRef_AddTestA != NULL )
    {
        _handlerRef_AddTestA( x, contextPtr );
    }
    else
    {
        LE_ERROR("ERROR in client _Handle_AddTestA: no registered handler");
    }

    // Release the message, now that we are finished with it.
    le_msg_ReleaseMsg(_msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * TestA handler ADD function
 */
//--------------------------------------------------------------------------------------------------
TestARef_t AddTestA
(
    TestAFunc_t handlerPtr,
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

    TestARef_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_AddTestA;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    // The input parameters are stored in the client data object, and it is
    // a pointer to this object that is passed down.
    // Create a new client data object and fill it in
    _ClientData_t* _clientDataPtr = le_mem_ForceAlloc(_ClientDataPool);
    _clientDataPtr->handlerPtr = (le_event_HandlerFunc_t)handlerPtr;
    _clientDataPtr->contextPtr = contextPtr;
    _clientDataPtr->callersThreadRef = le_thread_GetCurrent();
    contextPtr = _clientDataPtr;
    _msgBufPtr = PackData( _msgBufPtr, &contextPtr, sizeof(void*) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response");
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );
    // Put the handler reference result into the client data object, and
    // then return a safe reference to the client data object as the reference.
    _clientDataPtr->handlerRef = (le_event_HandlerRef_t)_result;
    _LOCK
    _result = le_ref_CreateRef(_HandlerRefMap, _clientDataPtr);
    _UNLOCK

    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);

    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * TestA handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void RemoveTestA
(
    TestARef_t addHandlerRef
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
    _msgPtr->id = _MSGID_RemoveTestA;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    // The passed in handlerRef is a safe reference for the client data object.  Need to get the
    // real handlerRef from the client data object and then delete both the safe reference and
    // the object since they are no longer needed.
    _LOCK
    _ClientData_t* clientDataPtr = le_ref_Lookup(_HandlerRefMap, addHandlerRef);
    le_ref_DeleteRef(_HandlerRefMap, addHandlerRef);
    _UNLOCK
    addHandlerRef = (TestARef_t)clientDataPtr->handlerRef;
    le_mem_Release(clientDataPtr);
    _msgBufPtr = PackData( _msgBufPtr, &addHandlerRef, sizeof(TestARef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response");
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
        ///< [IN]
        ///< first one-line comment
        ///< second one-line comment

    uint32_t* bPtr,
        ///< [OUT]

    const uint32_t* dataPtr,
        ///< [IN]

    size_t dataNumElements,
        ///< [IN]

    uint32_t* outputPtr,
        ///< [OUT]
        ///< some more comments here
        ///< and some comments here as well

    size_t* outputNumElementsPtr,
        ///< [INOUT]

    const char* label,
        ///< [IN]

    char* response,
        ///< [OUT]
        ///< comments on final parameter, first line
        ///< and more comments

    size_t responseNumElements,
        ///< [IN]

    char* more,
        ///< [OUT]
        ///< This parameter tests a bug fix

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
    LE_DEBUG("Sending message to server and waiting for response");
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
    _msgBufPtr = UnpackDataString( _msgBufPtr, response, responseNumElements*sizeof(char) );
    _msgBufPtr = UnpackDataString( _msgBufPtr, more, moreNumElements*sizeof(char) );

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
    LE_DEBUG("Sending message to server and waiting for response");
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
static void _Handle_AddBugTest
(
    void* _reportPtr,
    void* _notUsed
)
{
    le_msg_MessageRef_t _msgRef = _reportPtr;
    _Message_t* _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    uint8_t* _msgBufPtr = _msgPtr->buffer;

    // The clientContextPtr always exists and is always first.
    void* _clientContextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &_clientContextPtr, sizeof(void*) );

    // Pull out additional data from the context pointer
    _ClientData_t* _clientDataPtr = _clientContextPtr;
    BugTestFunc_t _handlerRef_AddBugTest = (BugTestFunc_t)_clientDataPtr->handlerPtr;
    void* contextPtr = _clientDataPtr->contextPtr;

    // Unpack the remaining parameters


    // Call the registered handler
    if ( _handlerRef_AddBugTest != NULL )
    {
        _handlerRef_AddBugTest( contextPtr );
    }
    else
    {
        LE_ERROR("ERROR in client _Handle_AddBugTest: no registered handler");
    }

    // Release the message, now that we are finished with it.
    le_msg_ReleaseMsg(_msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * BugTest handler ADD function
 */
//--------------------------------------------------------------------------------------------------
BugTestRef_t AddBugTest
(
    const char* newPathPtr,
        ///< [IN]

    BugTestFunc_t handlerPtr,
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

    BugTestRef_t _result;

    // Range check values, if appropriate
    if ( strlen(newPathPtr) > 512 ) LE_FATAL("strlen(newPathPtr) > 512");


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_AddBugTest;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackString( _msgBufPtr, newPathPtr );
    // The input parameters are stored in the client data object, and it is
    // a pointer to this object that is passed down.
    // Create a new client data object and fill it in
    _ClientData_t* _clientDataPtr = le_mem_ForceAlloc(_ClientDataPool);
    _clientDataPtr->handlerPtr = (le_event_HandlerFunc_t)handlerPtr;
    _clientDataPtr->contextPtr = contextPtr;
    _clientDataPtr->callersThreadRef = le_thread_GetCurrent();
    contextPtr = _clientDataPtr;
    _msgBufPtr = PackData( _msgBufPtr, &contextPtr, sizeof(void*) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response");
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );
    // Put the handler reference result into the client data object, and
    // then return a safe reference to the client data object as the reference.
    _clientDataPtr->handlerRef = (le_event_HandlerRef_t)_result;
    _LOCK
    _result = le_ref_CreateRef(_HandlerRefMap, _clientDataPtr);
    _UNLOCK

    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);

    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * BugTest handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void RemoveBugTest
(
    BugTestRef_t addHandlerRef
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
    _msgPtr->id = _MSGID_RemoveBugTest;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    // The passed in handlerRef is a safe reference for the client data object.  Need to get the
    // real handlerRef from the client data object and then delete both the safe reference and
    // the object since they are no longer needed.
    _LOCK
    _ClientData_t* clientDataPtr = le_ref_Lookup(_HandlerRefMap, addHandlerRef);
    le_ref_DeleteRef(_HandlerRefMap, addHandlerRef);
    _UNLOCK
    addHandlerRef = (BugTestRef_t)clientDataPtr->handlerRef;
    le_mem_Release(clientDataPtr);
    _msgBufPtr = PackData( _msgBufPtr, &addHandlerRef, sizeof(BugTestRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response");
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

    // Pull out the callers thread
    _ClientData_t* clientDataPtr = clientContextPtr;
    le_thread_Ref_t callersThreadRef = clientDataPtr->callersThreadRef;

    // Trigger the appropriate event
    switch (msgPtr->id)
    {
        case _MSGID_AddTestA :
            le_event_QueueFunctionToThread(callersThreadRef, _Handle_AddTestA, msgRef, NULL);
            break;

        case _MSGID_AddBugTest :
            le_event_QueueFunctionToThread(callersThreadRef, _Handle_AddBugTest, msgRef, NULL);
            break;

        default: LE_ERROR("Unknowm msg id = %i for client thread = %p", msgPtr->id, callersThreadRef);
    }
}

