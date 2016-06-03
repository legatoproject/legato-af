#
# C code generator functions for server implementation files
#
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#

import os
import cStringIO
import collections
import re

import common
import codeTypes

import codeGenCommon


#---------------------------------------------------------------------------------------------------
# Globals
#---------------------------------------------------------------------------------------------------

# Contains the text for the server files.
# These will be written out to actual files
ServerFileText = cStringIO.StringIO()



#---------------------------------------------------------------------------------------------------
# Server templates/code
#---------------------------------------------------------------------------------------------------


#---------------------------------------------------------------------------------------------------
# This function is the handler that is registered with the server-side AddHandler function.
# It will take the parameters and package them up for sending back to the client.
#---------------------------------------------------------------------------------------------------

FuncAsyncTemplate = """
static void AsyncResponse_{{func.name}}
(
    {{ handler.parmList | printParmList("clientParmList") | indent }}
)
{
    le_msg_MessageRef_t _msgRef;
    _Message_t* _msgPtr;
    _ServerData_t* serverDataPtr = (_ServerData_t*)contextPtr;

    {% if func.handlerName and not func.isAddHandler -%}
    // This is a one-time handler; if the server accidently calls it a second time, then
    // the client sesssion ref would be NULL.
    if ( serverDataPtr->clientSessionRef == NULL )
    {
        LE_FATAL("Handler passed to {{func.name}}() can't be called more than once");
    }
    {% endif %}

    // Will not be used if no data is sent back to client
    __attribute__((unused)) uint8_t* _msgBufPtr;

    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(serverDataPtr->clientSessionRef);
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_{{func.name}};
    _msgBufPtr = _msgPtr->buffer;

    // Always pack the client context pointer first
    _msgBufPtr = PackData( _msgBufPtr, &(serverDataPtr->contextPtr), sizeof(void*) );

    // Pack the input parameters
    {{ handler.parmList | printParmList("clientPack", sep="\n") | indent }}

    // Send the async response to the client
    LE_DEBUG("Sending message to client session %p : %ti bytes sent",
             serverDataPtr->clientSessionRef,
             _msgBufPtr-_msgPtr->buffer);
    SendMsgToClient(_msgRef);

    $ if func.handlerName and not func.isAddHandler :
    {{""}}
    // The registered handler has been called, so no longer need the server data.
    // Explicitly set clientSessionRef to NULL, so that we can catch if this function gets
    // accidently called again.
    serverDataPtr->clientSessionRef = NULL;
    le_mem_Release(serverDataPtr);
    $ endif
}
"""


def WriteAsyncFuncCode(func, handler, template):
    handlerStr = common.FormatCode(template, func=func, handler=handler)
    print >>ServerFileText, handlerStr



#---------------------------------------------------------------------------------------------------
# This function takes the message from the client, unpacks it, and calls the real server-side
# function, and then packs any return value or out parameters.
#---------------------------------------------------------------------------------------------------

FuncHandlerTemplate = """
static void Handle_{{func.name}}
(
    le_msg_MessageRef_t _msgRef

)
{
    // Get the message buffer pointer
    uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Needed if we are returning a result or output values
    uint8_t* _msgBufStartPtr = _msgBufPtr;

    // Unpack the input parameters from the message
    {{ func.parmListIn | printParmList("serverUnpack", sep="\n\n") | indent }}

    {% if func.handlerName -%}
    // Create a new server data object and fill it in
    _ServerData_t* serverDataPtr = le_mem_ForceAlloc(_ServerDataPool);
    serverDataPtr->clientSessionRef = le_msg_GetSession(_msgRef);
    serverDataPtr->contextPtr = contextPtr;
    serverDataPtr->handlerRef = NULL;
    serverDataPtr->removeHandlerFunc = NULL;
    contextPtr = serverDataPtr;
    {% endif %}

    // Define storage for output parameters
    {{ func.parmListOut | printParmList("serverParmList", sep="\n") | indent }}

    // Call the function
    {% if not func.type -%}

    {{func.name}} ( {{ func.parmList | printParmList("serverCallName", sep=", ") }} );

    {% else -%}

    {{func.type}} _result;
    _result = {{func.name}} ( {{ func.parmList | printParmList("serverCallName", sep=", ") }} );

    {% if func.isAddHandler -%}
    // Put the handler reference result and a pointer to the associated remove function
    // into the server data object.  This function pointer is needed in case the client
    // is closed and the handlers need to be removed.
    serverDataPtr->handlerRef = (le_event_HandlerRef_t)_result;
    serverDataPtr->removeHandlerFunc = (RemoveHandlerFunc_t){{func.name | replace("Add", "Remove", 1)}};

    // Return a safe reference to the server data object as the reference.
    _LOCK
    _result = le_ref_CreateRef(_HandlerRefMap, serverDataPtr);
    _UNLOCK
    {% endif %}

    {% endif %}

    // Re-use the message buffer for the response
    _msgBufPtr = _msgBufStartPtr;

    {% if func.type -%}
    // Pack the result first
    _msgBufPtr = PackData( _msgBufPtr, &_result, sizeof(_result) );
    {% endif %}

    // Pack any "out" parameters
    {{ func.parmListOut | printParmList("serverPack", sep="\n") | indent }}

    // Return the response
    LE_DEBUG("Sending response to client session %p : %ti bytes sent",
             le_msg_GetSession(_msgRef),
             _msgBufPtr-_msgBufStartPtr);
    le_msg_Respond(_msgRef);
}
"""


AsyncFuncHandlerTemplate = """
{{respondProto}}
{
    LE_ASSERT(_cmdRef != NULL);

    // Get the message related data
    le_msg_MessageRef_t _msgRef = (le_msg_MessageRef_t)_cmdRef;
    _Message_t* _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    __attribute__((unused)) uint8_t* _msgBufPtr = _msgPtr->buffer;

    // Ensure the passed in msgRef is for the correct message
    LE_ASSERT(_msgPtr->id == _MSGID_{{func.name}});

    // Ensure that this Respond function has not already been called
    LE_FATAL_IF( !le_msg_NeedsResponse(_msgRef), "Response has already been sent");

    {% if func.type -%}
    // Pack the result first
    _msgBufPtr = PackData( _msgBufPtr, &_result, sizeof(_result) );
    {% endif %}

    // Pack any "out" parameters
    {{ func.parmListOut | printParmList("asyncServerPack", sep="\n") | indent }}

    // Return the response
    LE_DEBUG("Sending response to client session %p", le_msg_GetSession(_msgRef));
    le_msg_Respond(_msgRef);
}

static void Handle_{{func.name}}
(
    le_msg_MessageRef_t _msgRef
)
{
    // Get the message buffer pointer
    __attribute__((unused)) uint8_t* _msgBufPtr = ((_Message_t*)le_msg_GetPayloadPtr(_msgRef))->buffer;

    // Unpack the input parameters from the message
    {{ func.parmListIn | printParmList("serverUnpack", sep="\n\n") | indent }}

    // Call the function
    {{func.name}} ( ({{ "ServerCmdRef_t" | addNamePrefix }})_msgRef
                    {{- func.parmListInCall | printParmList("asyncServerCallName", sep=", ", leadSep=True) }} );
}
"""


def WriteHandlerCode(func, template):
    # The prototype parameter is only needed for the AsyncFuncHandlerTemplate, but it does no
    # harm to always include it.  It will be ignored for the other template(s).
    funcStr = common.FormatCode(template,
                                func=func,
                                respondProto=codeGenCommon.GetRespondFuncPrototypeStr(func))
    print >>ServerFileText, funcStr



#---------------------------------------------------------------------------------------------------
# This function take the message from the client, and then based on the message ID, calls
# the appropriate handler.
#---------------------------------------------------------------------------------------------------

MsgHandlerTemplate = """
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
        $ for func in funcList
        case _MSGID_{{func.name}} : Handle_{{func.name}}(msgRef); break;
        $ endfor
        {{""}}
        default: LE_ERROR("Unknowm msg id = %i", msgPtr->id);
    }

    // Clear the client session ref associated with the current message, since the message
    // has now been processed.
    _ClientSessionRef = 0;
}
"""


def WriteMsgHandler(flist, template):
    print >>ServerFileText, common.FormatCode(template, funcList=flist)



#---------------------------------------------------------------------------------------------------
# Output file templates/code
#---------------------------------------------------------------------------------------------------


ServerGenericCode = """
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
"""


ServerStartFuncCode = """
{{ proto['getServiceRef'] }}
{
    return _ServerServiceRef;
}


{{ proto['getSessionRef'] }}
{
    return _ClientSessionRef;
}


{{ proto['startServerFunc'] }}
{
    LE_DEBUG("======= Starting Server %s ========", SERVICE_INSTANCE_NAME);

    le_msg_ProtocolRef_t protocolRef;

    // Create the server data pool
    _ServerDataPool = le_mem_CreatePool("{{ "ServerData" | addNamePrefix }}", sizeof(_ServerData_t));

    // Create safe reference map for handler references.
    // The size of the map should be based on the number of handlers defined for the server.
    // Don't expect that to be more than 2-3, so use 3 as a reasonable guess.
    _HandlerRefMap = le_ref_CreateMap("{{ "ServerHandlers" | addNamePrefix }}", 3);

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
"""


ServerStartClientCode = """
//--------------------------------------------------------------------------------------------------
// Client Specific Server Code
//--------------------------------------------------------------------------------------------------
"""


def WriteServerFile(headerFiles, pf, ph, genericFunctions, genAsync):
    codeGenCommon.WriteWarning(ServerFileText)

    print >>ServerFileText, '\n' + '\n'.join('#include "%s"'%h for h in headerFiles) + '\n'
    print >>ServerFileText, codeGenCommon.DefaultPackerUnpacker
    print >>ServerFileText, ServerGenericCode

    # Note that this does not need to be an ordered dictionary, unlike genericFunctions
    protoDict = { n: codeGenCommon.GetFuncPrototypeStr(f) for n,f in genericFunctions.items() }
    print >>ServerFileText, common.FormatCode(ServerStartFuncCode, proto=protoDict)

    print >>ServerFileText, ServerStartClientCode

    for f in pf.values():
        #print f
        if f.handlerName:
            # Write out the handler first; there should only be one per function
            for h in ph.values():
                if h.name == f.handlerName:
                    WriteAsyncFuncCode(f, h, FuncAsyncTemplate)
                    break

        # Write out the functions next.
        # Functions that have handler parameters, or are Add and Remove handler functions are
        # never asynchronous.
        if genAsync and not f.handlerName and not f.isRemoveHandler :
            WriteHandlerCode(f, AsyncFuncHandlerTemplate)
        else:
            WriteHandlerCode(f, FuncHandlerTemplate)

    WriteMsgHandler(pf.values(), MsgHandlerTemplate)

    return ServerFileText


