/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#include "messages.h"
#include "server.h"


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
// Generic Server Types, Variables and Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Type definition for generic function to remove a handler, given the handler ref.
 */
//--------------------------------------------------------------------------------------------------
typedef void(* RemoveHandlerFunc_t)(void *handlerRef);


//--------------------------------------------------------------------------------------------------
/**
 * Server Data Objects
 *
 * This object is used to store additional context info for each request
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t   clientSessionRef;     ///< The client to send the response to
    void*                 contextPtr;           ///< ContextPtr registered with handler
    le_event_HandlerRef_t handlerRef;           ///< HandlerRef for the registered handler
    RemoveHandlerFunc_t   removeHandlerFunc;    ///< Function to remove the registered handler
}
_ServerData_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for server data objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t _ServerDataPool;


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
 * Forward declaration needed by StartServer
 */
//--------------------------------------------------------------------------------------------------
static void ServerMsgRecvHandler
(
    le_msg_MessageRef_t msgRef,
    void*               contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Server Service Reference
 */
//--------------------------------------------------------------------------------------------------
static le_msg_ServiceRef_t _ServerServiceRef;


//--------------------------------------------------------------------------------------------------
/**
 * Server Thread Reference
 *
 * Reference to the thread that is registered to provide this service.
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t _ServerThreadRef;


//--------------------------------------------------------------------------------------------------
/**
 * Client Session Reference for the current message received from a client
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t _ClientSessionRef;


//--------------------------------------------------------------------------------------------------
/**
 * Cleanup client data if the client is no longer connected
 */
//--------------------------------------------------------------------------------------------------
static void CleanupClientData
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    LE_DEBUG("Client %p is closed !!!", sessionRef);

    // Iterate over the server data reference map and remove anything that matches
    // the client session.
    _LOCK

    // Store the client session ref so it can be retrieved by the server using the
    // GetClientSessionRef() function, if it's needed inside handler removal functions.
    _ClientSessionRef = sessionRef;

    le_ref_IterRef_t iterRef = le_ref_GetIterator(_HandlerRefMap);
    le_result_t result = le_ref_NextNode(iterRef);
    _ServerData_t const* serverDataPtr;

    while ( result == LE_OK )
    {
        serverDataPtr =  le_ref_GetValue(iterRef);

        if ( sessionRef != serverDataPtr->clientSessionRef )
        {
            LE_DEBUG("Found session ref %p; does not match",
                     serverDataPtr->clientSessionRef);
        }
        else
        {
            LE_DEBUG("Found session ref %p; match found, so needs cleanup",
                     serverDataPtr->clientSessionRef);

            // Remove the handler, if the Remove handler functions exists.
            if ( serverDataPtr->removeHandlerFunc != NULL )
            {
                serverDataPtr->removeHandlerFunc( serverDataPtr->handlerRef );
            }

            // Release the server data block
            le_mem_Release((void*)serverDataPtr);

            // Delete the associated safeRef
            le_ref_DeleteRef( _HandlerRefMap, (void*)le_ref_GetSafeRef(iterRef) );

            // Since the reference map was modified, the iterator is no longer valid and
            // so has to be re-initalized.  This means that some values may get revisited,
            // but eventually this will iterate over the whole reference map.
            // todo: Is there an easier way?
            iterRef = le_ref_GetIterator(_HandlerRefMap);
        }

        // Get the next value in the reference mpa
        result = le_ref_NextNode(iterRef);
    }

    // Clear the client session ref, since the event has now been processed.
    _ClientSessionRef = 0;

    _UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Send the message to the client (queued version)
 *
 * This is a wrapper around le_msg_Send() with an extra parameter so that it can be used
 * with le_event_QueueFunctionToThread().
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void SendMsgToClientQueued
(
    void*  msgRef,  ///< [in] Reference to the message.
    void*  unused   ///< [in] Not used
)
{
    le_msg_Send(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Send the message to the client.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void SendMsgToClient
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    /*
     * If called from a thread other than the server thread, queue the message onto the server
     * thread.  This is necessary to allow async response/handler functions to be called from any
     * thread, whereas messages to the client can only be sent from the server thread.
     */
    if ( le_thread_GetCurrent() != _ServerThreadRef )
    {
        le_event_QueueFunctionToThread(_ServerThreadRef,
                                       SendMsgToClientQueued,
                                       msgRef,
                                       NULL);
    }
    else
    {
        le_msg_Send(msgRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t GetServiceRef
(
    void
)
{
    return _ServerServiceRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t GetClientSessionRef
(
    void
)
{
    return _ClientSessionRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the server and advertise the service.
 */
//--------------------------------------------------------------------------------------------------
void AdvertiseService
(
    void
)
{
    LE_DEBUG("======= Starting Server %s ========", SERVICE_INSTANCE_NAME);

    le_msg_ProtocolRef_t protocolRef;

    // Create the server data pool
    _ServerDataPool = le_mem_CreatePool("ServerData", sizeof(_ServerData_t));

    // Create safe reference map for handler references.
    // The size of the map should be based on the number of handlers defined for the server.
    // Don't expect that to be more than 2-3, so use 3 as a reasonable guess.
    _HandlerRefMap = le_ref_CreateMap("ServerHandlers", 3);

    // Start the server side of the service
    protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID_STR, sizeof(_Message_t));
    _ServerServiceRef = le_msg_CreateService(protocolRef, SERVICE_INSTANCE_NAME);
    le_msg_SetServiceRecvHandler(_ServerServiceRef, ServerMsgRecvHandler, NULL);
    le_msg_AdvertiseService(_ServerServiceRef);

    // Register for client sessions being closed
    le_msg_AddServiceCloseHandler(_ServerServiceRef, CleanupClientData, NULL);

    // Need to keep track of the thread that is registered to provide this service.
    _ServerThreadRef = le_thread_GetCurrent();
}


//--------------------------------------------------------------------------------------------------
// Client Specific Server Code
//--------------------------------------------------------------------------------------------------


static void AsyncResponse_AddTestAHandler
(
    int32_t x,
    void* contextPtr
)
{
    le_msg_MessageRef_t _msgRef;
    _Message_t* _msgPtr;
    _ServerData_t* serverDataPtr = (_ServerData_t*)contextPtr;


    // Will not be used if no data is sent back to client
    __attribute__((unused)) uint8_t* _msgBufPtr;

    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(serverDataPtr->clientSessionRef);
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_AddTestAHandler;
    _msgBufPtr = _msgPtr->buffer;

    // Always pack the client context pointer first
    _msgBufPtr = PackData( _msgBufPtr, &(serverDataPtr->contextPtr), sizeof(void*) );

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &x, sizeof(int32_t) );

    // Send the async response to the client
    LE_DEBUG("Sending message to client session %p : %ti bytes sent",
             serverDataPtr->clientSessionRef,
             _msgBufPtr-_msgPtr->buffer);
    SendMsgToClient(_msgRef);
}


static void Handle_AddTestAHandler
(
    le_msg_MessageRef_t _msgRef

)
{
    // Get the message buffer pointer
    uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Needed if we are returning a result or output values
    uint8_t* _msgBufStartPtr = _msgBufPtr;

    // Unpack the input parameters from the message


    void* contextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &contextPtr, sizeof(void*) );

    // Create a new server data object and fill it in
    _ServerData_t* serverDataPtr = le_mem_ForceAlloc(_ServerDataPool);
    serverDataPtr->clientSessionRef = le_msg_GetSession(_msgRef);
    serverDataPtr->contextPtr = contextPtr;
    serverDataPtr->handlerRef = NULL;
    serverDataPtr->removeHandlerFunc = NULL;
    contextPtr = serverDataPtr;

    // Define storage for output parameters


    // Call the function
    TestAHandlerRef_t _result;
    _result = AddTestAHandler ( AsyncResponse_AddTestAHandler, contextPtr );

    // Put the handler reference result and a pointer to the associated remove function
    // into the server data object.  This function pointer is needed in case the client
    // is closed and the handlers need to be removed.
    serverDataPtr->handlerRef = (le_event_HandlerRef_t)_result;
    serverDataPtr->removeHandlerFunc = (RemoveHandlerFunc_t)RemoveTestAHandler;

    // Return a safe reference to the server data object as the reference.
    _LOCK
    _result = le_ref_CreateRef(_HandlerRefMap, serverDataPtr);
    _UNLOCK


    // Re-use the message buffer for the response
    _msgBufPtr = _msgBufStartPtr;

    // Pack the result first
    _msgBufPtr = PackData( _msgBufPtr, &_result, sizeof(_result) );

    // Pack any "out" parameters


    // Return the response
    LE_DEBUG("Sending response to client session %p : %ti bytes sent",
             le_msg_GetSession(_msgRef),
             _msgBufPtr-_msgBufStartPtr);
    le_msg_Respond(_msgRef);
}


static void Handle_RemoveTestAHandler
(
    le_msg_MessageRef_t _msgRef

)
{
    // Get the message buffer pointer
    uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Needed if we are returning a result or output values
    uint8_t* _msgBufStartPtr = _msgBufPtr;

    // Unpack the input parameters from the message
    TestAHandlerRef_t addHandlerRef;
    _msgBufPtr = UnpackData( _msgBufPtr, &addHandlerRef, sizeof(TestAHandlerRef_t) );
    // The passed in handlerRef is a safe reference for the server data object.  Need to get the
    // real handlerRef from the server data object and then delete both the safe reference and
    // the object since they are no longer needed.
    _LOCK
    _ServerData_t* serverDataPtr = le_ref_Lookup(_HandlerRefMap, addHandlerRef);
    if ( serverDataPtr == NULL )
    {
        _UNLOCK
        LE_KILL_CLIENT("Invalid reference");
        return;
    }
    le_ref_DeleteRef(_HandlerRefMap, addHandlerRef);
    _UNLOCK
    addHandlerRef = (TestAHandlerRef_t)serverDataPtr->handlerRef;
    le_mem_Release(serverDataPtr);


    // Define storage for output parameters


    // Call the function
    RemoveTestAHandler ( addHandlerRef );


    // Re-use the message buffer for the response
    _msgBufPtr = _msgBufStartPtr;


    // Pack any "out" parameters


    // Return the response
    LE_DEBUG("Sending response to client session %p : %ti bytes sent",
             le_msg_GetSession(_msgRef),
             _msgBufPtr-_msgBufStartPtr);
    le_msg_Respond(_msgRef);
}


static void Handle_allParameters
(
    le_msg_MessageRef_t _msgRef

)
{
    // Get the message buffer pointer
    uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Needed if we are returning a result or output values
    uint8_t* _msgBufStartPtr = _msgBufPtr;

    // Unpack the input parameters from the message
    common_EnumExample_t a;
    _msgBufPtr = UnpackData( _msgBufPtr, &a, sizeof(common_EnumExample_t) );

    size_t dataNumElements;
    _msgBufPtr = UnpackData( _msgBufPtr, &dataNumElements, sizeof(size_t) );

    uint32_t data[dataNumElements];
    _msgBufPtr = UnpackData( _msgBufPtr, data, dataNumElements*sizeof(uint32_t) );

    size_t outputNumElements;
    _msgBufPtr = UnpackData( _msgBufPtr, &outputNumElements, sizeof(size_t) );
    if ( outputNumElements > 10 )
    {
        LE_DEBUG("Adjusting outputNumElements from %zd to 10", outputNumElements);
        outputNumElements = 10;
    }

    char label[21];
    _msgBufPtr = UnpackString( _msgBufPtr, label, 21 );

    size_t responseNumElements;
    _msgBufPtr = UnpackData( _msgBufPtr, &responseNumElements, sizeof(size_t) );

    size_t moreNumElements;
    _msgBufPtr = UnpackData( _msgBufPtr, &moreNumElements, sizeof(size_t) );


    // Define storage for output parameters
    uint32_t b;

    uint32_t output[outputNumElements];
    char response[responseNumElements]; response[0]=0;
    char more[moreNumElements]; more[0]=0;

    // Call the function
    allParameters ( a, &b, data, dataNumElements, output, &outputNumElements, label, response, responseNumElements, more, moreNumElements );


    // Re-use the message buffer for the response
    _msgBufPtr = _msgBufStartPtr;


    // Pack any "out" parameters
    _msgBufPtr = PackData( _msgBufPtr, &b, sizeof(uint32_t) );
    _msgBufPtr = PackData( _msgBufPtr, &outputNumElements, sizeof(size_t) );
    _msgBufPtr = PackData( _msgBufPtr, output, outputNumElements*sizeof(uint32_t) );
    _msgBufPtr = PackString( _msgBufPtr, response );
    _msgBufPtr = PackString( _msgBufPtr, more );

    // Return the response
    LE_DEBUG("Sending response to client session %p : %ti bytes sent",
             le_msg_GetSession(_msgRef),
             _msgBufPtr-_msgBufStartPtr);
    le_msg_Respond(_msgRef);
}


static void Handle_FileTest
(
    le_msg_MessageRef_t _msgRef

)
{
    // Get the message buffer pointer
    uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Needed if we are returning a result or output values
    uint8_t* _msgBufStartPtr = _msgBufPtr;

    // Unpack the input parameters from the message
    int dataFile;
    dataFile = le_msg_GetFd(_msgRef);


    // Define storage for output parameters
    int dataOut;

    // Call the function
    FileTest ( dataFile, &dataOut );


    // Re-use the message buffer for the response
    _msgBufPtr = _msgBufStartPtr;


    // Pack any "out" parameters
    le_msg_SetFd(_msgRef, dataOut);

    // Return the response
    LE_DEBUG("Sending response to client session %p : %ti bytes sent",
             le_msg_GetSession(_msgRef),
             _msgBufPtr-_msgBufStartPtr);
    le_msg_Respond(_msgRef);
}


static void Handle_TriggerTestA
(
    le_msg_MessageRef_t _msgRef

)
{
    // Get the message buffer pointer
    uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Needed if we are returning a result or output values
    uint8_t* _msgBufStartPtr = _msgBufPtr;

    // Unpack the input parameters from the message



    // Define storage for output parameters


    // Call the function
    TriggerTestA (  );


    // Re-use the message buffer for the response
    _msgBufPtr = _msgBufStartPtr;


    // Pack any "out" parameters


    // Return the response
    LE_DEBUG("Sending response to client session %p : %ti bytes sent",
             le_msg_GetSession(_msgRef),
             _msgBufPtr-_msgBufStartPtr);
    le_msg_Respond(_msgRef);
}


static void AsyncResponse_AddBugTestHandler
(
    void* contextPtr
)
{
    le_msg_MessageRef_t _msgRef;
    _Message_t* _msgPtr;
    _ServerData_t* serverDataPtr = (_ServerData_t*)contextPtr;


    // Will not be used if no data is sent back to client
    __attribute__((unused)) uint8_t* _msgBufPtr;

    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(serverDataPtr->clientSessionRef);
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_AddBugTestHandler;
    _msgBufPtr = _msgPtr->buffer;

    // Always pack the client context pointer first
    _msgBufPtr = PackData( _msgBufPtr, &(serverDataPtr->contextPtr), sizeof(void*) );

    // Pack the input parameters


    // Send the async response to the client
    LE_DEBUG("Sending message to client session %p : %ti bytes sent",
             serverDataPtr->clientSessionRef,
             _msgBufPtr-_msgPtr->buffer);
    SendMsgToClient(_msgRef);
}


static void Handle_AddBugTestHandler
(
    le_msg_MessageRef_t _msgRef

)
{
    // Get the message buffer pointer
    uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Needed if we are returning a result or output values
    uint8_t* _msgBufStartPtr = _msgBufPtr;

    // Unpack the input parameters from the message
    char newPathPtr[513];
    _msgBufPtr = UnpackString( _msgBufPtr, newPathPtr, 513 );



    void* contextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &contextPtr, sizeof(void*) );

    // Create a new server data object and fill it in
    _ServerData_t* serverDataPtr = le_mem_ForceAlloc(_ServerDataPool);
    serverDataPtr->clientSessionRef = le_msg_GetSession(_msgRef);
    serverDataPtr->contextPtr = contextPtr;
    serverDataPtr->handlerRef = NULL;
    serverDataPtr->removeHandlerFunc = NULL;
    contextPtr = serverDataPtr;

    // Define storage for output parameters


    // Call the function
    BugTestHandlerRef_t _result;
    _result = AddBugTestHandler ( newPathPtr, AsyncResponse_AddBugTestHandler, contextPtr );

    // Put the handler reference result and a pointer to the associated remove function
    // into the server data object.  This function pointer is needed in case the client
    // is closed and the handlers need to be removed.
    serverDataPtr->handlerRef = (le_event_HandlerRef_t)_result;
    serverDataPtr->removeHandlerFunc = (RemoveHandlerFunc_t)RemoveBugTestHandler;

    // Return a safe reference to the server data object as the reference.
    _LOCK
    _result = le_ref_CreateRef(_HandlerRefMap, serverDataPtr);
    _UNLOCK


    // Re-use the message buffer for the response
    _msgBufPtr = _msgBufStartPtr;

    // Pack the result first
    _msgBufPtr = PackData( _msgBufPtr, &_result, sizeof(_result) );

    // Pack any "out" parameters


    // Return the response
    LE_DEBUG("Sending response to client session %p : %ti bytes sent",
             le_msg_GetSession(_msgRef),
             _msgBufPtr-_msgBufStartPtr);
    le_msg_Respond(_msgRef);
}


static void Handle_RemoveBugTestHandler
(
    le_msg_MessageRef_t _msgRef

)
{
    // Get the message buffer pointer
    uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Needed if we are returning a result or output values
    uint8_t* _msgBufStartPtr = _msgBufPtr;

    // Unpack the input parameters from the message
    BugTestHandlerRef_t addHandlerRef;
    _msgBufPtr = UnpackData( _msgBufPtr, &addHandlerRef, sizeof(BugTestHandlerRef_t) );
    // The passed in handlerRef is a safe reference for the server data object.  Need to get the
    // real handlerRef from the server data object and then delete both the safe reference and
    // the object since they are no longer needed.
    _LOCK
    _ServerData_t* serverDataPtr = le_ref_Lookup(_HandlerRefMap, addHandlerRef);
    if ( serverDataPtr == NULL )
    {
        _UNLOCK
        LE_KILL_CLIENT("Invalid reference");
        return;
    }
    le_ref_DeleteRef(_HandlerRefMap, addHandlerRef);
    _UNLOCK
    addHandlerRef = (BugTestHandlerRef_t)serverDataPtr->handlerRef;
    le_mem_Release(serverDataPtr);


    // Define storage for output parameters


    // Call the function
    RemoveBugTestHandler ( addHandlerRef );


    // Re-use the message buffer for the response
    _msgBufPtr = _msgBufStartPtr;


    // Pack any "out" parameters


    // Return the response
    LE_DEBUG("Sending response to client session %p : %ti bytes sent",
             le_msg_GetSession(_msgRef),
             _msgBufPtr-_msgBufStartPtr);
    le_msg_Respond(_msgRef);
}


static void AsyncResponse_TestCallback
(
    uint32_t data,
    const char* name,
    int dataFile,
    void* contextPtr
)
{
    le_msg_MessageRef_t _msgRef;
    _Message_t* _msgPtr;
    _ServerData_t* serverDataPtr = (_ServerData_t*)contextPtr;

    // This is a one-time handler; if the server accidently calls it a second time, then
    // the client sesssion ref would be NULL.
    if ( serverDataPtr->clientSessionRef == NULL )
    {
        LE_FATAL("Handler passed to TestCallback() can't be called more than once");
    }

    // Will not be used if no data is sent back to client
    __attribute__((unused)) uint8_t* _msgBufPtr;

    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(serverDataPtr->clientSessionRef);
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_TestCallback;
    _msgBufPtr = _msgPtr->buffer;

    // Always pack the client context pointer first
    _msgBufPtr = PackData( _msgBufPtr, &(serverDataPtr->contextPtr), sizeof(void*) );

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &data, sizeof(uint32_t) );
    _msgBufPtr = PackString( _msgBufPtr, name );
    le_msg_SetFd(_msgRef, dataFile);

    // Send the async response to the client
    LE_DEBUG("Sending message to client session %p : %ti bytes sent",
             serverDataPtr->clientSessionRef,
             _msgBufPtr-_msgPtr->buffer);
    SendMsgToClient(_msgRef);

    // The registered handler has been called, so no longer need the server data.
    // Explicitly set clientSessionRef to NULL, so that we can catch if this function gets
    // accidently called again.
    serverDataPtr->clientSessionRef = NULL;
    le_mem_Release(serverDataPtr);
}


static void Handle_TestCallback
(
    le_msg_MessageRef_t _msgRef

)
{
    // Get the message buffer pointer
    uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Needed if we are returning a result or output values
    uint8_t* _msgBufStartPtr = _msgBufPtr;

    // Unpack the input parameters from the message
    uint32_t someParm;
    _msgBufPtr = UnpackData( _msgBufPtr, &someParm, sizeof(uint32_t) );

    size_t dataArrayNumElements;
    _msgBufPtr = UnpackData( _msgBufPtr, &dataArrayNumElements, sizeof(size_t) );

    uint8_t dataArray[dataArrayNumElements];
    _msgBufPtr = UnpackData( _msgBufPtr, dataArray, dataArrayNumElements*sizeof(uint8_t) );



    void* contextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &contextPtr, sizeof(void*) );

    // Create a new server data object and fill it in
    _ServerData_t* serverDataPtr = le_mem_ForceAlloc(_ServerDataPool);
    serverDataPtr->clientSessionRef = le_msg_GetSession(_msgRef);
    serverDataPtr->contextPtr = contextPtr;
    serverDataPtr->handlerRef = NULL;
    serverDataPtr->removeHandlerFunc = NULL;
    contextPtr = serverDataPtr;

    // Define storage for output parameters


    // Call the function
    int32_t _result;
    _result = TestCallback ( someParm, dataArray, dataArrayNumElements, AsyncResponse_TestCallback, contextPtr );



    // Re-use the message buffer for the response
    _msgBufPtr = _msgBufStartPtr;

    // Pack the result first
    _msgBufPtr = PackData( _msgBufPtr, &_result, sizeof(_result) );

    // Pack any "out" parameters


    // Return the response
    LE_DEBUG("Sending response to client session %p : %ti bytes sent",
             le_msg_GetSession(_msgRef),
             _msgBufPtr-_msgBufStartPtr);
    le_msg_Respond(_msgRef);
}


static void Handle_TriggerCallbackTest
(
    le_msg_MessageRef_t _msgRef

)
{
    // Get the message buffer pointer
    uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Needed if we are returning a result or output values
    uint8_t* _msgBufStartPtr = _msgBufPtr;

    // Unpack the input parameters from the message
    uint32_t data;
    _msgBufPtr = UnpackData( _msgBufPtr, &data, sizeof(uint32_t) );


    // Define storage for output parameters


    // Call the function
    TriggerCallbackTest ( data );


    // Re-use the message buffer for the response
    _msgBufPtr = _msgBufStartPtr;


    // Pack any "out" parameters


    // Return the response
    LE_DEBUG("Sending response to client session %p : %ti bytes sent",
             le_msg_GetSession(_msgRef),
             _msgBufPtr-_msgBufStartPtr);
    le_msg_Respond(_msgRef);
}


static void ServerMsgRecvHandler
(
    le_msg_MessageRef_t msgRef,
    void*               contextPtr
)
{
    // Get the message payload so that we can get the message "id"
    _Message_t* msgPtr = le_msg_GetPayloadPtr(msgRef);

    // Get the client session ref for the current message.  This ref is used by the server to
    // get info about the client process, such as user id.  If there are multiple clients, then
    // the session ref may be different for each message, hence it has to be queried each time.
    _ClientSessionRef = le_msg_GetSession(msgRef);

    // Dispatch to appropriate message handler and get response
    switch (msgPtr->id)
    {
        case _MSGID_AddTestAHandler : Handle_AddTestAHandler(msgRef); break;
        case _MSGID_RemoveTestAHandler : Handle_RemoveTestAHandler(msgRef); break;
        case _MSGID_allParameters : Handle_allParameters(msgRef); break;
        case _MSGID_FileTest : Handle_FileTest(msgRef); break;
        case _MSGID_TriggerTestA : Handle_TriggerTestA(msgRef); break;
        case _MSGID_AddBugTestHandler : Handle_AddBugTestHandler(msgRef); break;
        case _MSGID_RemoveBugTestHandler : Handle_RemoveBugTestHandler(msgRef); break;
        case _MSGID_TestCallback : Handle_TestCallback(msgRef); break;
        case _MSGID_TriggerCallbackTest : Handle_TriggerCallbackTest(msgRef); break;

        default: LE_ERROR("Unknowm msg id = %i", msgPtr->id);
    }

    // Clear the client session ref associated with the current message, since the message
    // has now been processed.
    _ClientSessionRef = 0;
}

