// -------------------------------------------------------------------------------------------------
/**
 *  Voice Call Server
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 * todo:
 *  - assumes that there is a valid SIM and the modem is registered on the network
 *  - only handles the voice call on mobile network
 *  - has a very simple recovery mechanism after voice connection is lost; this needs improvement.
 */
// -------------------------------------------------------------------------------------------------
#include "legato.h"

#include "interfaces.h"
#include "watchdogChain.h"


// Todo => Add Option to set Sim Profile.
#define MAX_DESTINATION_LEN         50
#define MAX_DESTINATION_LEN_BYTE    51
#define AUDIO_MODE_LEN              15
#define AUDIO_MODE_BYTES            16

#define MAX_VOICECALL_PROFILE       1


//--------------------------------------------------------------------------------------------------
/**
 * Definitions for sending request,release,answer and release commands to voice call command thread
 */
//--------------------------------------------------------------------------------------------------
#define REQUEST_CALL_COMMAND        1
#define END_CALL_COMMAND            2
#define ANSWER_CALL_COMMAND         3

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

//--------------------------------------------------------------------------------------------------
/**
 * Event for sending connection to applications
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ConnStateEvent;


//--------------------------------------------------------------------------------------------------
/**
 * Event for sending Voice call command
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t CommandEvent;


//--------------------------------------------------------------------------------------------------
/**
 * MCC Voice call context profile.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
        le_mcc_CallRef_t                    callRef;
} MccContext_t;


//--------------------------------------------------------------------------------------------------
/**
 * Voice call context profile.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
        // Call object parameters
        le_voicecall_CallRef_t            callObjRef;      ///< Voice call object reference
        char                              destination[MAX_DESTINATION_LEN_BYTE];
        le_audio_StreamRef_t              rxStream;
        le_audio_StreamRef_t              txStream;
        le_voicecall_Event_t              lastEvent;
        le_voicecall_TerminationReason_t  lastTerminationReason;
        le_msg_SessionRef_t               sessionRef;      ///< Client sessionRef
        union
        {
                // Mcc call context parameters
                MccContext_t mcc;
                // TODO Add other call context for future uses with other service like VoIP
        };
} VoiceCallContext_t;


//--------------------------------------------------------------------------------------------------
/**
 * Voice call command structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
        uint32_t                           command;
        char                               destination[MAX_DESTINATION_LEN_BYTE];
        VoiceCallContext_t                 *callCtxPtr;
} CmdRequest_t;


//--------------------------------------------------------------------------------------------------
/**
 * Call event structure associated with the above ConnStateEvent.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
        le_voicecall_CallRef_t  callObjRef;
        char                    identifier[MAX_DESTINATION_LEN_BYTE];
        le_voicecall_Event_t    callEvent;
        void*                   ptr;
} VoiceCallState_t;



//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the request reference
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t VoiceCallRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference for pool request
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t VoiceCallPool = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference for hash map
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t VoiceCallCtxMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference for Mcc Call Handler reference.
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_CallEventHandlerRef_t MccCallEventHandlerRef;

//--------------------------------------------------------------------------------------------------
/**
 * Counter of Mcc Call Handler references.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t MccCallEventHandlerRefCount;



//--------------------------------------------------------------------------------------------------
/**
 * Send connection state event
 */
//--------------------------------------------------------------------------------------------------
static void SendConnStateEvent
(
    VoiceCallContext_t * call_ctx,
    le_voicecall_Event_t event
)
{
    VoiceCallState_t eventVoiceCall;

    eventVoiceCall.callObjRef = call_ctx->callObjRef;
    le_utf8_Copy(eventVoiceCall.identifier, call_ctx->destination, MAX_DESTINATION_LEN_BYTE, NULL);
    eventVoiceCall.callEvent = event;

    // Send the event to interested applications
    le_event_Report(ConnStateEvent, &eventVoiceCall, sizeof(eventVoiceCall));
}


// -------------------------------------------------------------------------------------------------
/**
 *  Retrieve voice call context from a call ObjRef reference.
 *
 *  return:
 *      VoiceCallContext_t if found
 *       If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
// -------------------------------------------------------------------------------------------------
static VoiceCallContext_t * GetCallContextFromCallref
(
    le_mcc_CallRef_t callRef
)
{
    VoiceCallContext_t * ctxPtr = le_hashmap_Get(VoiceCallCtxMap, callRef);

    LE_WARN_IF(!ctxPtr, "Could not retrieve VoiceCall context from reference %p", callRef);

    return ctxPtr;
}


// -------------------------------------------------------------------------------------------------
/**
 *  Event callback for voice session state changes.
 */
// -------------------------------------------------------------------------------------------------
static void VoiceSessionStateHandler
(
    le_mcc_CallRef_t callRef,
    le_mcc_Event_t callEvent,
    void* contextPtr
)
{
    switch (callEvent)
    {
        case LE_MCC_EVENT_ALERTING:
        {
            VoiceCallContext_t * ctxPtr = GetCallContextFromCallref(callRef);

            LE_DEBUG("Call event is LE_MCC_EVENT_ALERTING.");
            if (ctxPtr)
            {
                // Send the state event to applications
                ctxPtr->lastEvent = LE_VOICECALL_EVENT_ALERTING;
                SendConnStateEvent(ctxPtr, ctxPtr->lastEvent);
            }
        }
        break;

        case LE_MCC_EVENT_INCOMING:
        {
            VoiceCallContext_t * newCtxPtr;

            LE_DEBUG("Call event is LE_MCC_EVENT_INCOMING.");
            // Create a new voice call object.
            newCtxPtr = (VoiceCallContext_t *) le_mem_TryAlloc(VoiceCallPool);

            // Resource available to create a new voice call context.
            if (newCtxPtr)
            {
                memset(newCtxPtr, 0, sizeof(VoiceCallContext_t));
                // Need to return a unique reference that will be used by
                // le_voicecall_GetTerminationReason() or le_voicecall_End().
                newCtxPtr->callObjRef = le_ref_CreateRef(VoiceCallRefMap, (void*) newCtxPtr);
                LE_DEBUG(" le_ref_CreateRef newCtxPtr 0x%p , callObjRef 0x%p",newCtxPtr,
                    newCtxPtr->callObjRef);

                // Retrieves call references.
                newCtxPtr->mcc.callRef = callRef;

                // Retrieves remote identifier.
                le_mcc_GetRemoteTel(newCtxPtr->mcc.callRef, newCtxPtr->destination,
                    MAX_DESTINATION_LEN_BYTE);

                // Create an entry for reference of the link between mcc callRef and object context.
                le_hashmap_Put(VoiceCallCtxMap, newCtxPtr->mcc.callRef, newCtxPtr);
                LE_DEBUG(" le_hashmap_Put callRef 0x%p , newCtxPtr 0x%p",newCtxPtr->mcc.callRef,
                    newCtxPtr);

                // Send incoming notification event to application.
                newCtxPtr->lastEvent = LE_VOICECALL_EVENT_INCOMING;
                SendConnStateEvent(newCtxPtr, newCtxPtr->lastEvent);
            }
            else
            {
                // No more resource available to create a new voice call context.
                VoiceCallContext_t ctx_temp;

                LE_WARN("No more resource ");
                ctx_temp.callObjRef = NULL;
                strncpy(ctx_temp.destination, "TODO", MAX_DESTINATION_LEN_BYTE);
                ctx_temp.lastEvent = LE_VOICECALL_EVENT_RESOURCE_BUSY;

                // Send error event to application.
                SendConnStateEvent(&ctx_temp, ctx_temp.lastEvent);
            }
        }
        break;

        case LE_MCC_EVENT_CONNECTED:
        {
            VoiceCallContext_t * ctxPtr = GetCallContextFromCallref(callRef);

            LE_DEBUG("Call event is LE_MCC_EVENT_CONNECTED.");
            if (ctxPtr)
            {
                ctxPtr->lastEvent = LE_VOICECALL_EVENT_CONNECTED;
                SendConnStateEvent(ctxPtr, ctxPtr->lastEvent );
            }
        }
        break;

        case LE_MCC_EVENT_ORIGINATING:
        {
            LE_DEBUG("Call event is LE_MCC_EVENT_ORIGINATING.");
        }
        break;

        case LE_MCC_EVENT_TERMINATED:
        {
            VoiceCallContext_t * ctxPtr = GetCallContextFromCallref(callRef);

            if (ctxPtr)
            {
                le_mcc_TerminationReason_t term = le_mcc_GetTerminationReason(
                    ctxPtr->mcc.callRef);

                ctxPtr->lastEvent = LE_VOICECALL_EVENT_TERMINATED;

                switch (term)
                {
                    case LE_MCC_TERM_NETWORK_FAIL:
                    {
                        LE_DEBUG("Termination reason is LE_MCC_TERM_NETWORK_FAIL");
                        ctxPtr->lastTerminationReason = LE_VOICECALL_TERM_NETWORK_FAIL;
                    }
                    break;

                    case LE_MCC_TERM_UNASSIGNED_NUMBER:
                    {
                        LE_DEBUG("Termination reason is LE_MCC_TERM_UNASSIGNED_NUMBER");
                        ctxPtr->lastTerminationReason = LE_VOICECALL_TERM_BAD_ADDRESS;
                    }
                    break;

                    case LE_MCC_TERM_USER_BUSY:
                    {
                        LE_DEBUG("Termination reason is LE_MCC_TERM_USER_BUSY");
                        ctxPtr->lastTerminationReason = LE_VOICECALL_TERM_BUSY;
                    }
                    break;

                    case LE_MCC_TERM_LOCAL_ENDED:
                    {
                        LE_DEBUG("Termination reason is LE_MCC_TERM_LOCAL_ENDED");
                        ctxPtr->lastTerminationReason = LE_VOICECALL_TERM_LOCAL_ENDED;
                    }
                    break;

                    case LE_MCC_TERM_REMOTE_ENDED:
                    {
                        LE_DEBUG("Termination reason is LE_MCC_TERM_REMOTE_ENDED");
                        ctxPtr->lastTerminationReason = LE_VOICECALL_TERM_REMOTE_ENDED;
                    }
                    break;

                    case LE_MCC_TERM_UNDEFINED:
                    {
                        LE_DEBUG("Termination reason is LE_MCC_TERM_UNDEFINED");
                        ctxPtr->lastTerminationReason = LE_VOICECALL_TERM_UNDEFINED;
                    }
                    break;

                    default:
                    {
                        LE_DEBUG("Termination reason is %d", term);
                        ctxPtr->lastTerminationReason = LE_VOICECALL_TERM_UNDEFINED;
                    }
                    break;
                }

                le_mcc_Delete(ctxPtr->mcc.callRef);
                // Send LE_VOICECALL_EVENT_TERMINATED event to application with the reason.
                SendConnStateEvent(ctxPtr, ctxPtr->lastEvent);
            }
            else
            {
                LE_ERROR("Context for callRef not found %p", callRef);
                le_mcc_Delete(callRef);
            }
        }
        break;

        default:
        {
            LE_WARN("Unknown Call event. %d", callEvent);
        }
        break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Answer voice session
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AnswerVoiceSession
(
    VoiceCallContext_t * ctxPtr
)
{
    return le_mcc_Answer(ctxPtr->mcc.callRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start voice session
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartVoiceSession
(
    VoiceCallContext_t * ctxPtr
)
{
    ctxPtr->mcc.callRef = le_mcc_Create(ctxPtr->destination);

    le_hashmap_Put(VoiceCallCtxMap, ctxPtr->mcc.callRef, ctxPtr);
    LE_DEBUG(" le_hashmap_Put callRef 0x%p , ctxPtr 0x%p",ctxPtr->mcc.callRef, ctxPtr);

    return le_mcc_Start(ctxPtr->mcc.callRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Disconnect the audio path and end voice call
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StopVoiceSession
(
    VoiceCallContext_t * ctxPtr
)
{
    le_result_t res = LE_OK;

    if (ctxPtr->lastEvent == LE_VOICECALL_EVENT_TERMINATED)
    {
        LE_WARN("Voice call already terminated callRef %p", ctxPtr->mcc.callRef);
    }
    else
    {
        res = le_mcc_HangUp(ctxPtr->mcc.callRef);
    }

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a command
 */
//--------------------------------------------------------------------------------------------------
static void ProcessCommand
(
    void* msgCommand
)
{
    uint32_t command = ((CmdRequest_t*) msgCommand)->command;
    VoiceCallContext_t * call_ctx = ((CmdRequest_t*) msgCommand)->callCtxPtr;

    switch (command)
    {
        case REQUEST_CALL_COMMAND:
        {
            LE_DEBUG("VoiceCallProcessCommand REQUEST_CALL_COMMAND");
            snprintf(call_ctx->destination, MAX_DESTINATION_LEN_BYTE, "%s",
                ((CmdRequest_t*) msgCommand)->destination);
            le_result_t res = StartVoiceSession(call_ctx);
            if (res != LE_OK)
            {
                SendConnStateEvent(call_ctx, LE_VOICECALL_EVENT_RESOURCE_BUSY);
            }
        }
        break;

        case END_CALL_COMMAND:
        {
            LE_DEBUG("VoiceCallProcessCommand END_CALL_COMMAND");
            le_result_t res = StopVoiceSession(call_ctx);
            if (res != LE_OK)
            {
                SendConnStateEvent(call_ctx, LE_VOICECALL_EVENT_CALL_END_FAILED);
            }
        }
        break;

        case ANSWER_CALL_COMMAND:
        {
            LE_DEBUG("VoiceCallProcessCommand ANSWER_CALL_COMMAND");
            le_result_t res = AnswerVoiceSession(call_ctx);
            if (res != LE_OK)
            {
                SendConnStateEvent(call_ctx, LE_VOICECALL_EVENT_CALL_ANSWER_FAILED);
            }
        }
        break;

        default:
        {
            LE_ERROR("Command %i is not valid", command);
        }
        break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Connection State Handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerStateHandler
(
    void* reportPtr, void* secondLayerHandlerFunc
)
{
    VoiceCallState_t* eventVoicePtr = reportPtr;
    le_voicecall_StateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(eventVoicePtr->callObjRef, eventVoicePtr->identifier,
        eventVoicePtr->callEvent,
        le_event_GetContextPtr());
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function to release hashmap reference and delete object reference
 */
//--------------------------------------------------------------------------------------------------
static void VoiceCallPoolDestructor
(
    void * objPtr
)
{
    VoiceCallContext_t * ctxPtr = objPtr;

    LE_DEBUG("le_ref_DeleteRef(callObjRef 0x%p), hashRemove (callRef 0x%p)", ctxPtr->callObjRef,
        ctxPtr->mcc.callRef);

    le_ref_DeleteRef(VoiceCallRefMap, ctxPtr->callObjRef);
    le_hashmap_Remove(VoiceCallCtxMap, ctxPtr->mcc.callRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function to free resources when a client application is terminated
 *
 */
//--------------------------------------------------------------------------------------------------
 static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef, ///< [IN] Message session reference.
    void* contextPtr                ///< [IN] Context pointer.
)
{
    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    LE_DEBUG("SessionRef (%p) has been closed", sessionRef);

    le_ref_IterRef_t iter = le_ref_GetIterator(VoiceCallRefMap);
    while (LE_OK == le_ref_NextNode(iter))
    {
        VoiceCallContext_t* ctxPtr = (VoiceCallContext_t* )le_ref_GetValue(iter);
        if (ctxPtr)
        {
            if ((ctxPtr->sessionRef == sessionRef) || (MccCallEventHandlerRefCount == 0))
            {
                if (LE_OK != StopVoiceSession(ctxPtr))
                {
                    LE_WARN("Unable to stop an ongoing call");
                }

                LE_DEBUG("Release allocated resources");
                le_mem_Release(ctxPtr);
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * le_voicecall_StateHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_voicecall_StateHandlerRef_t le_voicecall_AddStateHandler
(
    le_voicecall_StateHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("VoiceConnState", ConnStateEvent,
                    FirstLayerStateHandler, (le_event_HandlerFunc_t) handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    if ( MccCallEventHandlerRef == NULL)
    {
        MccCallEventHandlerRef = le_mcc_AddCallEventHandler(VoiceSessionStateHandler,
                                                                NULL);
        LE_DEBUG("Mcc Call Event handler added");
    }

    MccCallEventHandlerRefCount++;

    return (le_voicecall_StateHandlerRef_t) (handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * le_voicecall_StateHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_voicecall_RemoveStateHandler
(
    le_voicecall_StateHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t) addHandlerRef);

    MccCallEventHandlerRefCount--;

    if (MccCallEventHandlerRefCount == 0)
    {
        le_mcc_RemoveCallEventHandler(MccCallEventHandlerRef);
        MccCallEventHandlerRef = NULL;
        LE_DEBUG("Mcc Call Event handler removed");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Start a voice call.
 *
 * @return
 *      - Reference to the voice call (to be used later for releasing the voice call)
 *      - NULL if the voice call could not be processed
 */
//--------------------------------------------------------------------------------------------------
le_voicecall_CallRef_t le_voicecall_Start
(
    const char* DestinationID
        ///< [IN]
        ///< Destination identifier for the voice call establishment.
)
{
    CmdRequest_t msgCommand;
    le_voicecall_CallRef_t callObjRef = NULL;

    msgCommand.callCtxPtr = (VoiceCallContext_t *) le_mem_TryAlloc(VoiceCallPool);

    if (msgCommand.callCtxPtr)
    {
        memset(msgCommand.callCtxPtr, 0, sizeof(VoiceCallContext_t));
        msgCommand.callCtxPtr->sessionRef = le_voicecall_GetClientSessionRef();
        msgCommand.command = REQUEST_CALL_COMMAND;
        le_utf8_Copy(msgCommand.destination, DestinationID, MAX_DESTINATION_LEN_BYTE, NULL);

        // Need to return a unique reference that will be used by
        // le_voicecall_GetTerminationReason() or le_voicecall_End().  Don't actually have any
        // voice for now, but have to use some value other than NULL for the voice pointer.
        callObjRef = le_ref_CreateRef(VoiceCallRefMap, (void*) msgCommand.callCtxPtr);
        LE_DEBUG("Create callObjRef 0x%p (Ctx 0x%p) ",callObjRef, msgCommand.callCtxPtr);

        if (callObjRef)
        {
            msgCommand.callCtxPtr->callObjRef = callObjRef;
            le_event_Report(CommandEvent, &msgCommand, sizeof(msgCommand));
        }
        else
        {
            LE_WARN("New reference can't be allocated");
        }
    }
    else
    {
        LE_WARN("New profile can't be allocated");
    }

    return callObjRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release a voice call.
 *
 * @return
 *      - LE_OK if the end of voice call can be processed.
 *      - LE_NOT_FOUND if the voice call object reference is not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_voicecall_End
(
    le_voicecall_CallRef_t reference
        ///< [IN]
        ///< Voice call object reference to hang-up.
)
{
    LE_DEBUG("le_voicecall_End 0x%p", reference);

    // Look up the reference.  If it is NULL, then the reference is not valid.
    // Otherwise, delete the reference and send the release command to the voice thread.
    VoiceCallContext_t * ctxPtr = le_ref_Lookup(VoiceCallRefMap, (void*) reference);

    if ( (ctxPtr == NULL) || (reference != ctxPtr->callObjRef))
    {
        LE_ERROR("Invalid voice request reference %p", reference);
        return LE_NOT_FOUND;
    }
    else
    {
        CmdRequest_t msgCommand;
        msgCommand.command = END_CALL_COMMAND;
        msgCommand.callCtxPtr = ctxPtr;
        le_event_Report(CommandEvent, &msgCommand, sizeof(msgCommand));
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Answer to incoming voice call.
 *
 * @return
 *      - LE_OK if the incoming voice call can be answered
 *      - LE_NOT_FOUND if the incoming voice call object reference is not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_voicecall_Answer
(
    le_voicecall_CallRef_t reference
        ///< [IN]
        ///< Incoming voice call object reference to answer.
)
{
    le_result_t result;

    // Look up the reference.  If it is NULL, then the reference is not valid.
    // Otherwise, send the answer command to the voice thread.
    VoiceCallContext_t * ctxPtr = le_ref_Lookup(VoiceCallRefMap, (void*) reference);

    if ( (ctxPtr == NULL) || (reference != ctxPtr->callObjRef))
    {
        LE_ERROR("Invalid voice request reference %p", reference);
        result = LE_NOT_FOUND;
    }
    else
    {
        CmdRequest_t msgCommand;
        msgCommand.command = ANSWER_CALL_COMMAND;
        msgCommand.callCtxPtr = ctxPtr;
        msgCommand.callCtxPtr->sessionRef = le_voicecall_GetClientSessionRef();

        le_event_Report(CommandEvent, &msgCommand, sizeof(msgCommand));
        result = LE_OK;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the termination reason of a voice call reference.
 *
 * @return
 *      - LE_OK if the termination reason is got
 *      - LE_NOT_FOUND if the incoming voice call object reference is not found.
 *      - LE_FAULT if the voice call is not terminated.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_voicecall_GetTerminationReason
(
    le_voicecall_CallRef_t reference,
        ///< [IN]
        ///< Voice call object reference to read from.

    le_voicecall_TerminationReason_t* reasonPtr
        ///< [OUT]
        ///< Termination reason of the voice call.
)
{
    if (reasonPtr == NULL)
    {
        LE_KILL_CLIENT("reasonPtr is NULL.");
        return LE_FAULT;
    }

    le_result_t result;

    VoiceCallContext_t * ctxPtr = le_ref_Lookup(VoiceCallRefMap, reference);

    if ( (ctxPtr == NULL) || (reference != ctxPtr->callObjRef))
    {
        LE_ERROR("Invalid voice call reference %p", reference);
        result = LE_NOT_FOUND;
    }
    else
    {
        LE_DEBUG("ctxPtr->lastEvent %d, ctxPtr->lastTerminationReason %d",ctxPtr->lastEvent,
                        ctxPtr->lastTerminationReason);
        if (ctxPtr->lastEvent == LE_VOICECALL_EVENT_TERMINATED)
        {
            *reasonPtr = ctxPtr->lastTerminationReason;
            result = LE_OK;
        }
        else
        {
            result = LE_FAULT;
        }
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete voice call object reference create by le_voicecall_Start() or an incoming voice call.
 *
 * @return
 *      - LE_OK if the delete of voice call can be processed.
 *      - LE_FAULT if the voice call is not terminated.
 *      - LE_NOT_FOUND if the voice call object reference is not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_voicecall_Delete
(
    le_voicecall_CallRef_t reference
        ///< [IN]
        ///< Voice call object reference to hang-up.
)
{
    le_result_t result;

    // Look up the reference.  If it is NULL, then the reference is not valid.
    // Otherwise, send the answer command to the voice thread.
    VoiceCallContext_t * ctxPtr = le_ref_Lookup(VoiceCallRefMap, (void*) reference);

    if ((ctxPtr == NULL) || (reference != ctxPtr->callObjRef))
    {
        LE_ERROR("Invalid voice call reference %p", reference);
        result = LE_NOT_FOUND;
    }
    else
    {
        if (ctxPtr->lastEvent == LE_VOICECALL_EVENT_TERMINATED)
        {
            le_mem_Release(ctxPtr);
            result = LE_OK;
        }
        else
        {
            LE_ERROR("The voice call is not terminated");
            result = LE_FAULT;
        }
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Called to get the received audio stream. All audio received from the
 * other end of the call is received on this stream.
 *
 * @return Received audio stream reference.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_voicecall_GetRxAudioStream
(
    le_voicecall_CallRef_t reference
        ///< [IN]
        ///< Voice call object reference to read from.
)
{
    // Look up the reference.  If it is NULL, then the reference is not valid.
    // Otherwise, delete the reference and send the release command to the voice thread.
    VoiceCallContext_t * ctxPtr = le_ref_Lookup(VoiceCallRefMap, (void*) reference);

    if ((ctxPtr == NULL) || (reference != ctxPtr->callObjRef))
    {
        LE_ERROR("Invalid voice call reference %p", reference);
        return NULL;
    }

    if (ctxPtr->lastEvent != LE_VOICECALL_EVENT_CONNECTED)
    {
        LE_WARN("Not in LE_VOICECALL_EVENT_CONNECTED state");
    }

    if (!ctxPtr->rxStream)
    {
        ctxPtr->rxStream = le_audio_OpenModemVoiceRx();
    }

    return ctxPtr->rxStream;
}


//--------------------------------------------------------------------------------------------------
/**
 * Called to get the transmitted audio stream. All audio generated on this
 * end of the call is sent on this stream.
 *
 * @return Transmitted audio stream reference.
  */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_voicecall_GetTxAudioStream
(
    le_voicecall_CallRef_t reference
        ///< [IN]
        ///< Voice call object reference to read from.
)
{
    // Look up the reference.  If it is NULL, then the reference is not valid.
    // Otherwise, delete the reference and send the release command to the voice thread.
    VoiceCallContext_t * ctxPtr = le_ref_Lookup(VoiceCallRefMap, (void*) reference);

    if ((ctxPtr == NULL) || (reference != ctxPtr->callObjRef))
    {
        LE_ERROR("Invalid voice call reference %p", reference);
        return NULL;
    }

    if (ctxPtr->lastEvent != LE_VOICECALL_EVENT_CONNECTED)
    {
        LE_WARN("Not in LE_VOICECALL_EVENT_CONNECTED state");
    }

    if (!ctxPtr->txStream)
    {
        ctxPtr->txStream = le_audio_OpenModemVoiceTx();
    }

    return ctxPtr->txStream;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Server Init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Init the Mcc Call Handler reference and handler counter.
    MccCallEventHandlerRef = NULL;
    MccCallEventHandlerRefCount = 0;

    // Create Pool to save Call contexts.
    VoiceCallPool = le_mem_CreatePool("CallServicePool", sizeof(VoiceCallContext_t));

    // Extend pool to max call allowed MAX_VOICECALL_PROFILE.
    le_mem_ExpandPool(VoiceCallPool, MAX_VOICECALL_PROFILE);

    // Set Destructor function to release hashmap reference.
    le_mem_SetDestructor(VoiceCallPool, VoiceCallPoolDestructor);

    // Init the various events
    CommandEvent = le_event_CreateId("Voice call Command", sizeof(CmdRequest_t));
    ConnStateEvent = le_event_CreateId("Voice call State", sizeof(VoiceCallState_t));

    // Create the voice call context indexer
    VoiceCallCtxMap = le_hashmap_Create  ( "VoiceCallIndexer", MAX_VOICECALL_PROFILE*2,
                    le_hashmap_HashVoidPointer, le_hashmap_EqualsVoidPointer);

    // Create safe reference map for request references. The size of the map should be based on
    // the expected number of simultaneous voice call requests.
    VoiceCallRefMap = le_ref_CreateMap("voiceRequests", MAX_VOICECALL_PROFILE*2);

    // Register close session handler.
    le_msg_AddServiceCloseHandler(le_voicecall_GetServiceRef(), CloseSessionEventHandler, NULL);

    // Register for command events
    le_event_AddHandler("VoiceCallProcessCommand", CommandEvent, ProcessCommand);

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);

    LE_INFO("Voice Call Service is ready");
}
