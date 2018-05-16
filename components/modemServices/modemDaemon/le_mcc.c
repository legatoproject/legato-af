//--------------------------------------------------------------------------------------------------
/**
 * @file le_mcc.c
 *
 * This file contains the source code of the high level MCC (Modem Call Control) API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------


#include "legato.h"
#include "interfaces.h"
#include "pa_mcc.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Call objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MCC_MAX_CALL    20

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of session objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MCC_MAX_SESSION 5

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum size of various profile related fields.
 */
//--------------------------------------------------------------------------------------------------
#define MCC_PROFILE_NAME_MAX_LEN    LE_MCC_PROFILE_NAME_MAX_LEN
#define MCC_PROFILE_NAME_MAX_BYTES  (MCC_PROFILE_NAME_MAX_LEN+1)

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Modem Call object structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_Call
{
    char                            telNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES]; ///< Telephone number
    int16_t                         callId;            ///< Outgoing call ID
    le_mcc_Event_t                  event;             ///< Last Call event
    le_mcc_Event_t                  lastEvent;         ///< Backup of last Call event
    le_mcc_TerminationReason_t      termination;       ///< Call termination reason
    int32_t                         terminationCode;   ///< Platform specific termination code
    pa_mcc_clir_t                   clirStatus;        ///< Call CLIR status
    bool                            inProgress;        ///< call in progress
    int16_t                         refCount;          ///< ref count
    le_dls_List_t                   creatorList;       ///< Clients sessionRef list
    le_dls_Link_t                   link;              ///< link for CallList
}
le_mcc_Call_t;

//--------------------------------------------------------------------------------------------------
/**
 * SessionRef node structure used for the creatorList list.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t sessionRef;           ///< client sessionRef
    le_dls_Link_t       link;                 ///< link for creatorList
}
SessionRefNode_t;

//--------------------------------------------------------------------------------------------------
/**
 * session context node structure used for the SessionCtxList list.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t sessionRef;           ///< client sessionRef
    le_dls_List_t       callRefList;          /// Call reference list
    le_dls_List_t       handlerList;          /// handler list
    le_dls_Link_t       link;                 ///< link for SessionCtxList
}
SessionCtxNode_t;

//--------------------------------------------------------------------------------------------------
/**
 * callRef node structure used for the callRefList list.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_mcc_CallRef_t callRef;           ///< The call reference.
    le_dls_Link_t    link;              ///< link for callRefList
}
CallRefNode_t;

//--------------------------------------------------------------------------------------------------
/**
 * HandlerCtx node structure used for the handlerList list.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_mcc_CallEventHandlerRef_t  handlerRef;      ///< handler reference
    le_mcc_CallEventHandlerFunc_t handlerFuncPtr;  ///< handler function
    void*                         userContext;     ///< user context
    SessionCtxNode_t*             sessionCtxPtr;   ///< session context relative to this handler ctx
    le_dls_Link_t                 link;            ///< link for handlerList
}
HandlerCtxNode_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for the clients sessionRef objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SessionRefPool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Calls.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   MccCallPool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for handlers context.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   HandlerPool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for sessions context.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   SessionCtxPool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for callRef context.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   CallRefPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Calls objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t MccCallRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for handlers objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t HandlerRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * list of session context.
 *
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t  SessionCtxList;

//--------------------------------------------------------------------------------------------------
/**
 * list of call context.
 *
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t  CallList;

//--------------------------------------------------------------------------------------------------
/**
 * Wakeup source to keep system awake during phone calls.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_pm_WakeupSourceRef_t WakeupSource = NULL;
#define CALL_WAKEUP_SOURCE_NAME "call"

//--------------------------------------------------------------------------------------------------
/**
 * Call destructor.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CallDestructor
(
    void* objPtr
)
{
    le_mcc_Call_t *callPtr = (le_mcc_Call_t*)objPtr;

    // Remove from the list
    if (callPtr)
    {
        le_dls_Remove(&CallList, &callPtr->link);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a call object.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_Call_t* GetCallObject
(
    const char*                     destinationPtr,
    int16_t                         id
)
{
    le_dls_Link_t* linkPtr = NULL;

    if (id != -1)
    {
        linkPtr = le_dls_Peek(&CallList);

        while ( linkPtr )
        {
            le_mcc_Call_t* callPtr = CONTAINER_OF( linkPtr,
                                                   le_mcc_Call_t,
                                                   link);

            linkPtr = le_dls_PeekNext(&CallList, linkPtr);

            // Check callId
            if ( (callPtr->callId == id) && callPtr->inProgress )
            {
                LE_DEBUG("callId found in callPtr %p", callPtr);
                return callPtr;
            }
        }
    }

    linkPtr = le_dls_Peek(&CallList);

    while ( linkPtr )
    {
        le_mcc_Call_t* callPtr = CONTAINER_OF( linkPtr,
                                               le_mcc_Call_t,
                                               link);

        linkPtr = le_dls_PeekNext(&CallList, linkPtr);

        // check phone number
        if ( strncmp(destinationPtr, callPtr->telNumber, sizeof(callPtr->telNumber)) == 0 )
        {
            LE_DEBUG("telNumber found in callPtr %p", callPtr);
            return callPtr;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a call object.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_Call_t* CreateCallObject
(
    const char*                     destinationPtr,
    int16_t                         id,
    le_mcc_Event_t                  event,
    le_mcc_TerminationReason_t      termination,
    int32_t                         terminationCode
)
{
    le_mcc_Call_t* callPtr = NULL;

    // New call
    callPtr = (le_mcc_Call_t*)le_mem_ForceAlloc(MccCallPool);

    if (!callPtr)
    {
        LE_ERROR("callPtr null !!!!");
        return NULL;
    }

    le_utf8_Copy(callPtr->telNumber, destinationPtr, sizeof(callPtr->telNumber), NULL);
    callPtr->callId = id;
    callPtr->event = event;
    callPtr->lastEvent = event;
    callPtr->termination = termination;
    callPtr->terminationCode = terminationCode;
    callPtr->clirStatus = PA_MCC_NO_CLIR;
    callPtr->inProgress = false;
    callPtr->refCount = 1;
    callPtr->creatorList=LE_DLS_LIST_INIT;
    callPtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&CallList, &callPtr->link);

    return callPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a session context.
 *
 */
//--------------------------------------------------------------------------------------------------
static SessionCtxNode_t* CreateSessionCtx
(
    void
)
{
    // Create the session context
    SessionCtxNode_t* sessionCtxPtr = le_mem_ForceAlloc(SessionCtxPool);
    sessionCtxPtr->sessionRef = le_mcc_GetClientSessionRef();
    sessionCtxPtr->link = LE_DLS_LINK_INIT;
    sessionCtxPtr->callRefList = LE_DLS_LIST_INIT;
    sessionCtxPtr->handlerList = LE_DLS_LIST_INIT;

    le_dls_Queue(&SessionCtxList, &(sessionCtxPtr->link));

    LE_DEBUG("Context for sessionRef %p created at %p", sessionCtxPtr->sessionRef, sessionCtxPtr);

    return sessionCtxPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a session context.
 *
 */
//--------------------------------------------------------------------------------------------------
static SessionCtxNode_t* GetSessionCtx
(
    le_msg_SessionRef_t sessionRef
)
{
    SessionCtxNode_t* sessionCtxPtr = NULL;
    le_dls_Link_t* linkPtr = le_dls_Peek(&SessionCtxList);

    while ( linkPtr )
    {
        SessionCtxNode_t* sessionCtxTmpPtr = CONTAINER_OF(linkPtr,
                                                        SessionCtxNode_t,
                                                        link);

        linkPtr = le_dls_PeekNext(&SessionCtxList, linkPtr);

        if ( sessionCtxTmpPtr->sessionRef == sessionRef )
        {
            sessionCtxPtr = sessionCtxTmpPtr;
        }
    }

    LE_DEBUG("sessionCtx %p found for the sessionRef %p", sessionCtxPtr, sessionRef);

    return sessionCtxPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the call was created by the required client.
 *
 */
//--------------------------------------------------------------------------------------------------
static bool IsCallCreatedByClient
(
    le_mcc_Call_t* callPtr,
    le_msg_SessionRef_t sessionRef
)
{
    le_dls_Link_t* linkPtr = NULL;

    if ( callPtr )
    {
        linkPtr = le_dls_Peek(&(callPtr->creatorList));
    }

    while ( linkPtr )
    {
        SessionRefNode_t* sessionRefNodePtr = CONTAINER_OF(linkPtr, SessionRefNode_t, link);
        linkPtr = le_dls_PeekNext(&(callPtr->creatorList), linkPtr);

        if (sessionRefNodePtr->sessionRef == sessionRef)
        {
            LE_DEBUG("callPtr %p created by sessionRef %p", callPtr, sessionRef);
            return true;
        }
    }

    LE_DEBUG("callPtr %p didn't create by sessionRef %p", callPtr, sessionRef);
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the call's reference for a specific client.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_CallRef_t GetCallRefFromSessionCtx
(
    le_mcc_Call_t* callPtr,
    SessionCtxNode_t* sessionCtxPtr
)
{
    le_dls_Link_t* linkPtr = NULL;

    if (sessionCtxPtr)
    {
        // Search from the tail, in order to always get the latest created callRef,
        // as the latest created callRef is the correct one for the latest call
        linkPtr = le_dls_PeekTail(&sessionCtxPtr->callRefList);
    }

    while( linkPtr )
    {
        CallRefNode_t* callRefPtr = CONTAINER_OF(   linkPtr,
                                                    CallRefNode_t,
                                                    link );

        linkPtr = le_dls_PeekPrev(&sessionCtxPtr->callRefList, linkPtr);

        le_mcc_Call_t* callTmpPtr = le_ref_Lookup(MccCallRefMap, callRefPtr->callRef);

        // Call found
        if ( callPtr == callTmpPtr )
        {
            LE_DEBUG("Call ref found: %p for callPtr %p and session %p",
                                                    callRefPtr->callRef, callPtr, sessionCtxPtr );
            return callRefPtr->callRef;
        }
    }

    LE_DEBUG("Call ref not found for callPtr %p and session %p", callPtr, sessionCtxPtr );
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the session context from the callRef.
 *
 */
//--------------------------------------------------------------------------------------------------
static SessionCtxNode_t* GetSessionCtxFromCallRef
(
    le_mcc_CallRef_t callRef
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&SessionCtxList);

    // For all sessions, search the callRef
    while ( linkPtr )
    {
        SessionCtxNode_t* sessionCtxPtr = CONTAINER_OF(linkPtr,
                                                        SessionCtxNode_t,
                                                        link);

        linkPtr = le_dls_PeekNext(&SessionCtxList, linkPtr);

        le_dls_Link_t* linkSessionCtxPtr = le_dls_Peek(&(sessionCtxPtr->callRefList));

        while( linkSessionCtxPtr )
        {
            CallRefNode_t* callRefNodePtr = CONTAINER_OF(linkSessionCtxPtr,
                                                        CallRefNode_t,
                                                        link);

            linkSessionCtxPtr = le_dls_PeekNext(&(sessionCtxPtr->callRefList), linkSessionCtxPtr);

            if ( callRefNodePtr->callRef == callRef )
            {
                LE_DEBUG("sessionCtx %p found for callRef %p",sessionCtxPtr, callRef);
                return sessionCtxPtr;
            }
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the call reference for a client.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_CallRef_t SetCallRefForSessionCtx
(
    le_mcc_Call_t* callPtr,
    SessionCtxNode_t* sessionCtxPtr
)
{
    CallRefNode_t* callNodePtr = le_mem_ForceAlloc(CallRefPool);

    if ( !callNodePtr || !sessionCtxPtr )
    {
        return NULL;
    }

    callNodePtr->callRef = le_ref_CreateRef(MccCallRefMap, callPtr);
    callNodePtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&sessionCtxPtr->callRefList, &callNodePtr->link);

    LE_DEBUG("Set %p for call %p and session %p", callNodePtr->callRef, callPtr, sessionCtxPtr);

    return callNodePtr->callRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Call all subscribed handlers.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CallHandlers
(
    le_mcc_Call_t* callPtr,
    bool newCall
)
{
    le_dls_Link_t* linkPtr = le_dls_PeekTail(&SessionCtxList);

    // For all sessions, call all handlers
    while ( linkPtr )
    {
        SessionCtxNode_t* sessionCtxPtr = CONTAINER_OF(linkPtr,
                                                        SessionCtxNode_t,
                                                        link);

        linkPtr = le_dls_PeekPrev(&SessionCtxList, linkPtr);

        LE_DEBUG("loop for sessionRef %p", sessionCtxPtr->sessionRef);

        // Peek the tail of the handlers list: this is important for handlers subscribed by
        // modemDaemon: MyCallEventHandler must be called the last one as this handler delete the
        // reference for modemDaemon
        le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&sessionCtxPtr->handlerList);

        // Nothing to do if no handler
        if (linkHandlerPtr)
        {
            // Search the callRef for this session
            le_mcc_CallRef_t callRef = GetCallRefFromSessionCtx(callPtr, sessionCtxPtr);

            if (callRef == NULL)
            {
                // callRef doesn't exist for this session => create it
                callRef = SetCallRefForSessionCtx(callPtr, sessionCtxPtr);

                // for incoming call (i.e. callPtr created into NewCallEventHandler), the first
                // callRef is not associated to a session. so we can use it (no addRef needed
                // in this case).
                // for other callRef created, we need to add a reference on the call context
                if (newCall)
                {
                    newCall = false;
                }
                else
                {
                    callPtr->refCount++;
                    le_mem_AddRef(callPtr);
                    LE_DEBUG("callRef created %p for call %p, count = %d",
                                                               callRef, callPtr, callPtr->refCount);
                }
            }
            else if ((callPtr->lastEvent == LE_MCC_EVENT_TERMINATED) &&
                     (IsCallCreatedByClient(callPtr, sessionCtxPtr->sessionRef) == false))
            {
                // This call was already used for an incoming call, but not deleted yet:
                // create a new callRef for the current call but not to reuse the previous
                // one, because the previous callRef is expected to be deleted soon.
                callRef = SetCallRefForSessionCtx(callPtr, sessionCtxPtr);
                callPtr->refCount++;
                le_mem_AddRef(callPtr);
                LE_DEBUG("callRef created %p for call %p, count = %d",
                                                               callRef, callPtr, callPtr->refCount);
            }
            else
            {
                LE_DEBUG("callRef found %p for call %p, count = %d",
                                                               callRef, callPtr, callPtr->refCount);
            }

            // if callRef exists, call the handler
            if (callRef != NULL)
            {
                // Itinerate on the handler list of the session
                while ( linkHandlerPtr )
                {
                    HandlerCtxNode_t * handlerCtxPtr = CONTAINER_OF(linkHandlerPtr,
                                                                    HandlerCtxNode_t,
                                                                    link);

                    linkHandlerPtr = le_dls_PeekPrev(&sessionCtxPtr->handlerList, linkHandlerPtr);

                    // Call the handler
                    LE_DEBUG("call handler for sessionRef %p, callRef %p",
                                            sessionCtxPtr->sessionRef,
                                            callRef);

                    handlerCtxPtr->handlerFuncPtr( callRef,
                                                   callPtr->event,
                                                   handlerCtxPtr->userContext );
                }
            }
            else
            {
                LE_ERROR("Null callRef !!!");
            }
        }
        else
        {
            LE_DEBUG("sessionCtxPtr %p has no handler", sessionCtxPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove a creator from a call
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveCreatorFromCall
(
    le_mcc_Call_t* callPtr,
    le_msg_SessionRef_t sessionRef
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&(callPtr->creatorList));

    while ( linkPtr )
    {
        SessionRefNode_t* sessionRefNodePtr = CONTAINER_OF(linkPtr, SessionRefNode_t, link);
        linkPtr = le_dls_PeekNext(&(callPtr->creatorList), linkPtr);

        if (sessionRefNodePtr->sessionRef == sessionRef)
        {
            LE_DEBUG("Remove sessionRef %p from callPtr %p", sessionRef, callPtr);
            le_dls_Remove(&(callPtr->creatorList), &(sessionRefNodePtr->link));
            le_mem_Release(sessionRefNodePtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove a call reference from a session context
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveCallRefFromSessionCtx
(
    SessionCtxNode_t* sessionCtxPtr,
    le_mcc_CallRef_t    callRef
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&sessionCtxPtr->callRefList);

    while ( linkPtr )
    {
        CallRefNode_t* CallRefPtr = CONTAINER_OF( linkPtr,
                                                  CallRefNode_t,
                                                  link);

        linkPtr = le_dls_PeekNext(&sessionCtxPtr->callRefList, linkPtr);

        if ( CallRefPtr->callRef == callRef )
        {
            // Remove this node
            LE_DEBUG("Remove callRef %p from sessionCtxPtr %p", callRef, sessionCtxPtr);
            le_ref_DeleteRef(MccCallRefMap, CallRefPtr->callRef);
            le_dls_Remove(&sessionCtxPtr->callRefList, &(CallRefPtr->link));
            le_mem_Release(CallRefPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Count the number of ongoing call
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t CountOngoingCall
(
    void
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&CallList);
    uint32_t counter = 0;

    while ( linkPtr )
    {
        le_mcc_Call_t* callPtr = CONTAINER_OF( linkPtr,
                                               le_mcc_Call_t,
                                               link);

        linkPtr = le_dls_PeekNext(&CallList, linkPtr);

        if ( callPtr->event != LE_MCC_EVENT_TERMINATED )
        {
            counter++;
        }
    }

    return counter;
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal Call event Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NewCallEventHandler
(
    pa_mcc_CallEventData_t*  dataPtr
)
{
    le_mcc_Call_t* callPtr = NULL;
    bool newCall = false;

    LE_DEBUG("call id %d, event %d", dataPtr->callId, dataPtr->event);

    // Acquire wakeup source on first indication of call
    if ((LE_MCC_EVENT_SETUP == dataPtr->event ||
        LE_MCC_EVENT_ORIGINATING == dataPtr->event ||
        LE_MCC_EVENT_INCOMING == dataPtr->event) &&
        ( CountOngoingCall() == 0 ))
    {
        le_pm_StayAwake(WakeupSource);
    }

    // Check if we have already an ongoing callPtr for this call
    callPtr = GetCallObject("", dataPtr->callId);

    if (callPtr == NULL)
    {
        // Call not in progress
        // check if a callPtr exists with the same number
        callPtr = GetCallObject(dataPtr->phoneNumber, -1);

        if (callPtr == NULL)
        {
            callPtr = CreateCallObject (dataPtr->phoneNumber,
                                        dataPtr->callId,
                                        dataPtr->event,
                                        dataPtr->terminationEvent,
                                        dataPtr->terminationCode);

            newCall = true;
        }
        else if (-1 == callPtr->callId)
        {
            // Call with no call id in MCC, update it with the value from PA
            callPtr->callId = dataPtr->callId;
        }

        callPtr->inProgress = true;
        callPtr->lastEvent = callPtr->event;
        callPtr->event = dataPtr->event;
        callPtr->termination = dataPtr->terminationEvent;
        callPtr->terminationCode = dataPtr->terminationCode;
    }
    else
    {
        if (callPtr->event == dataPtr->event)
        {
            // Get Phone number on the secondary Incoming event for CDMA incoming Call if available.
            if (dataPtr->event == LE_MCC_EVENT_INCOMING)
            {
                le_utf8_Copy(callPtr->telNumber, dataPtr->phoneNumber,
                    sizeof(callPtr->telNumber), NULL);
                LE_DEBUG("Phone number %s", callPtr->telNumber);
            }
            else
            {
                LE_DEBUG("Discard event %d for callPtr %p", callPtr->event, callPtr);
            }
            return;
        }
        callPtr->lastEvent = callPtr->event;
        callPtr->event = dataPtr->event;
        callPtr->termination = dataPtr->terminationEvent;
        callPtr->terminationCode = dataPtr->terminationCode;
    }

    // Handle call state transition
    switch (dataPtr->event)
    {
         case LE_MCC_EVENT_TERMINATED:
            // Release wakeup source once call termination is processed
            if (CountOngoingCall() == 0)
            {
                le_pm_Relax(WakeupSource);
            }
            callPtr->inProgress = false;
        break;
        default:
        break;
    }

    LE_DEBUG("callId %d event %d", callPtr->callId, callPtr->event);
    // Call the clients' handlers
    CallHandlers(callPtr, newCall);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications. Useful to remove the callRef of an incoming call
 * when no client subscribed to the service
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyCallEventHandler
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent,
    void* contextPtr
)
{
    if (callEvent == LE_MCC_EVENT_TERMINATED)
    {
        le_mcc_Delete(callRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * handler function to the close session service
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    // clean session context
    SessionCtxNode_t* sessionCtxPtr = GetSessionCtx(sessionRef);

    if (sessionCtxPtr)
    {
        le_dls_Link_t* linkPtr = le_dls_Pop(&sessionCtxPtr->callRefList);

        while ( linkPtr )
        {
            CallRefNode_t* CallRefPtr = CONTAINER_OF( linkPtr,
                                                      CallRefNode_t,
                                                      link);

            le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, CallRefPtr->callRef);
            if (NULL == callPtr)
            {
                LE_ERROR("Invalid reference (%p) provided!", CallRefPtr->callRef);
                return;
            }

            le_ref_DeleteRef(MccCallRefMap, CallRefPtr->callRef);
            le_mem_Release(CallRefPtr);

            // Remove the sessionRef from the call context
            RemoveCreatorFromCall(callPtr, sessionRef);

            callPtr->refCount--;

            LE_DEBUG("Release call %p countRef %d", callPtr, callPtr->refCount);

            LE_FATAL_IF((callPtr->refCount < 0),
                        "Error Release call %p, refCount %d",
                         callPtr, callPtr->refCount);

            le_mem_Release(callPtr);

            linkPtr = le_dls_Pop(&sessionCtxPtr->callRefList);
        }

        if (le_dls_NumLinks(&sessionCtxPtr->handlerList) == 0 )
        {
            le_dls_Remove(&SessionCtxList, &(sessionCtxPtr->link));
            le_mem_Release(sessionCtxPtr);
        }
    // Otherwise, the clean will be done during the handlers removal
    }
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the Modem Call Control
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Init
(
    void
)
{
    // Create a pool for Call objects
    MccCallPool = le_mem_CreatePool("MccCallPool", sizeof(struct le_mcc_Call));
    le_mem_ExpandPool(MccCallPool, MCC_MAX_CALL);
    le_mem_SetDestructor(MccCallPool, CallDestructor);

    SessionRefPool = le_mem_CreatePool("SessionRefPool", sizeof(SessionRefNode_t));
    le_mem_ExpandPool(SessionRefPool, MCC_MAX_CALL);

    HandlerPool = le_mem_CreatePool("HandlerPool", sizeof(HandlerCtxNode_t));
    le_mem_ExpandPool(HandlerPool, MCC_MAX_SESSION);

    CallRefPool = le_mem_CreatePool("CallRefPool", sizeof(CallRefNode_t));
    le_mem_ExpandPool(CallRefPool, MCC_MAX_SESSION*MCC_MAX_CALL);

    SessionCtxPool = le_mem_CreatePool("SessionCtxPool", sizeof(SessionCtxNode_t));
    le_mem_ExpandPool(SessionCtxPool, MCC_MAX_SESSION);

    HandlerRefMap = le_ref_CreateMap("HandlerRefMap", MCC_MAX_SESSION);

    // Create the Safe Reference Map to use for Call object Safe References.
    MccCallRefMap = le_ref_CreateMap("MccCallMap", MCC_MAX_CALL);

    // Initialize call wakeup source - succeeds or terminates caller
    WakeupSource = le_pm_NewWakeupSource(0, CALL_WAKEUP_SOURCE_NAME);

    SessionCtxList = LE_DLS_LIST_INIT;
    CallList = LE_DLS_LIST_INIT;

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler(le_mcc_GetServiceRef(),
                                  CloseSessionEventHandler,
                                  NULL);

    // Add an internal call handler
    le_mcc_AddCallEventHandler(MyCallEventHandler, NULL);

    // Register a handler function for Call Event indications
    if(pa_mcc_SetCallEventHandler(NewCallEventHandler) != LE_OK)
    {
        LE_CRIT("Add pa_mcc_SetCallEventHandler failed");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a call reference.
 *
 * @note Return NULL if call reference can't be created
 *
 * @note If destination number is too long (max LE_MDMDEFS_PHONE_NUM_MAX_LEN digits),
 * it is a fatal error, the function will not return.
 *
 */
//--------------------------------------------------------------------------------------------------
le_mcc_CallRef_t le_mcc_Create
(
    const char* phoneNumPtr
        ///< [IN]
        ///< The target number we are going to
        ///< call.
)
{
    if(strlen(phoneNumPtr) > (LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1))
    {
        LE_KILL_CLIENT("strlen(phoneNumPtr) > %d", (LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1));
        return NULL;
    }

    // Get the Call object.
    le_mcc_Call_t* mccCallPtr = GetCallObject(phoneNumPtr, -1);
    SessionCtxNode_t* sessionCtxPtr = GetSessionCtx(le_mcc_GetClientSessionRef());

    if (!sessionCtxPtr)
    {
        // Create the session context
        sessionCtxPtr = CreateSessionCtx();

        if (!sessionCtxPtr)
        {
            LE_ERROR("Impossible to create the session context");
            return NULL;
        }
    }

    // Call object already exists => do not allocate a new one
    if (mccCallPtr != NULL)
    {
        // Check if the client has already create this callRef
        // If yes, search the call reference into its session context
        if ( !IsCallCreatedByClient(mccCallPtr, le_mcc_GetClientSessionRef()) )
        {
            // same call created by a different client
            le_mem_AddRef(mccCallPtr);
            mccCallPtr->refCount++;
        }
        else
        {
            // already allocated by the current client

            // Return the callRef
            return GetCallRefFromSessionCtx(mccCallPtr, sessionCtxPtr);
        }
    }
    else
    {
        mccCallPtr = CreateCallObject (phoneNumPtr,
                                        -1,
                                        LE_MCC_EVENT_TERMINATED,
                                        LE_MCC_TERM_UNDEFINED,
                                        -1);
    }

    // Manage client session
    SessionRefNode_t* newSessionRefPtr = le_mem_ForceAlloc(SessionRefPool);
    newSessionRefPtr->sessionRef = le_mcc_GetClientSessionRef();
    newSessionRefPtr->link = LE_DLS_LINK_INIT;

    LE_DEBUG("Add a link in mccCallPtr %p for sessionRef %p",
                                                mccCallPtr, newSessionRefPtr->sessionRef);

    // Add the new sessionRef into the creator list
    le_dls_Queue(&mccCallPtr->creatorList,
                    &(newSessionRefPtr->link));

    // Add the call in the session context
    // Return a Safe Reference for this Call object.
    return SetCallRefForSessionCtx(mccCallPtr, sessionCtxPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Call to free up a call reference.
 *
 * @return
 *     - LE_OK        The function succeed.
 *     - LE_NOT_FOUND The call reference was not found.
 *     - LE_FAULT      The function failed.
 *
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Delete
(
    le_mcc_CallRef_t callRef   ///< [IN] The call object to free.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_ERROR("Invalid reference (%p) provided!", callRef);
        return LE_NOT_FOUND;
    }

    SessionCtxNode_t* sessionCtxPtr = GetSessionCtxFromCallRef(callRef);

    if ( !sessionCtxPtr )
    {
        LE_ERROR("No sessionCtx found for callRef %p !!!", callRef);
        return LE_FAULT;
    }

    LE_DEBUG("Delete callRef %p callPtr %p", callRef, callPtr);

    // Delete the callRef when the corresponding call is not in progress,
    // or this callRef is for a previously ended but not yet deleted call
    // (sometimes the delete event may delay).
    if ((callPtr->inProgress) &&
        (callRef == GetCallRefFromSessionCtx(callPtr, sessionCtxPtr)))
    {
        LE_ERROR("Call in progress !!");
        return LE_FAULT;
    }
    else
    {
        // Remove corresponding node from the creatorList
        RemoveCreatorFromCall(callPtr, sessionCtxPtr->sessionRef);

        // Remove the callRef from the sessionCtx
        RemoveCallRefFromSessionCtx(sessionCtxPtr, callRef);

        if ( ( le_dls_NumLinks(&sessionCtxPtr->handlerList) == 0 ) &&
                 ( le_dls_NumLinks(&sessionCtxPtr->callRefList) == 0 ) )
        {
            LE_DEBUG("Remove sessionCtxPtr %p", sessionCtxPtr);
            // delete the session context as it is not used anymore
            le_dls_Remove(&SessionCtxList, &(sessionCtxPtr->link));
            le_mem_Release(sessionCtxPtr);
        }

        callPtr->refCount--;
        LE_DEBUG("Release call %p, refCount %d", callPtr, callPtr->refCount);

        LE_FATAL_IF((callPtr->refCount < 0),
                    "Error Release call %p, refCount %d",
                    callPtr, callPtr->refCount);
        le_mem_Release(callPtr);

        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a call attempt.
 *
 * Asynchronous due to possible time to connect.
 *
 * As the call attempt proceeds, the profile's registered call event handler receives events.
 *
 * @return
 *      - LE_OK            Function succeed.
 *      - LE_BUSY          The call is already in progress
 *
 * * @note As this is an asynchronous call, a successful only confirms a call has been
 *       started. Don't assume a call has been successful yet.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Start
(
    le_mcc_CallRef_t callRef   ///< [IN] Reference to the call object.
)
{
    uint8_t        callId;
    le_result_t    res;
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return LE_NOT_FOUND;
    }

    if (callPtr->telNumber[0] == '\0')
    {
        LE_KILL_CLIENT("callPtr->telNumber is not set !");
        return LE_NOT_FOUND;
    }

    if ( callPtr->inProgress )
    {
        LE_INFO("Call already in progress");
        return LE_BUSY;
    }

    res = pa_mcc_VoiceDial(callPtr->telNumber,
                          callPtr->clirStatus,
                          PA_MCC_NO_CUG,
                          &callId,
                          &callPtr->termination);

    if ( res == LE_OK )
    {
        callPtr->callId = (int16_t)callId;
        callPtr->inProgress = true;
    }

    return res;

}

//--------------------------------------------------------------------------------------------------
/**
 * Allow the caller to know if the given call is actually connected or not.
 *
 * @return TRUE if the call is connected, FALSE otherwise.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
bool le_mcc_IsConnected
(
    le_mcc_CallRef_t  callRef  ///< [IN] The call reference to read.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return false;
    }

    LE_DEBUG("callRef %p, callId %d, event %d", callRef, callPtr->callId, callPtr->event);

    if (callPtr->event == LE_MCC_EVENT_CONNECTED)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read out the remote party telephone number associated with the call in question.
 *
 * Output parameter is updated with the telephone number. If the Telephone number string length exceeds
 * the value of 'len' parameter, the LE_OVERFLOW error code is returned and 'telPtr' is used until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'telPtr'.
 * Note that 'len' sould be at least equal to LE_MDMDEFS_PHONE_NUM_MAX_BYTES, otherwise LE_OVERFLOW
 * error code will be common.
 *
 * @return LE_OVERFLOW      The Telephone number length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_GetRemoteTel
(
    le_mcc_CallRef_t callRef,    ///< [IN]  The call reference to read from.
    char*                telPtr,     ///< [OUT] The telephone number string.
    size_t               len         ///< [IN]  The length of telephone number string.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return LE_NOT_FOUND;
    }
    if (telPtr == NULL)
    {
        LE_KILL_CLIENT("telPtr is NULL !");
        return LE_FAULT;
    }

    return (le_utf8_Copy(telPtr, callPtr->telNumber, len, NULL));
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the termination reason.
 *
 * @return The termination reason.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_TerminationReason_t le_mcc_GetTerminationReason
(
    le_mcc_CallRef_t callRef   ///< [IN] The call reference to read from.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return LE_MCC_TERM_UNDEFINED;
    }

    return (callPtr->termination);
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the platform specific termination code.
 *
 * Refer to @ref platformConstraintsSpecificErrorCodes for platform specific
 * termination code description.
 *
 * @return The platform specific termination code.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_mcc_GetPlatformSpecificTerminationCode
(
    le_mcc_CallRef_t callRef   ///< [IN] The call reference to read from.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return -1;
    }

    return (callPtr->terminationCode);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Answers incoming call.
 *
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Answer
(
    le_mcc_CallRef_t callRef   ///< [IN] The call reference.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return LE_NOT_FOUND;
    }

    return pa_mcc_Answer(callPtr->callId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect, or hang up, the specifed call. Any active call handlers
 * will be notified.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_HangUp
(
    le_mcc_CallRef_t callRef   ///< [IN] The call to end.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return LE_NOT_FOUND;
    }

    if (callPtr->inProgress)
    {
        return (pa_mcc_HangUp(callPtr->callId));
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function will disconnect, or hang up all the ongoing calls. Any active call handlers will
 * be notified.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_HangUpAll
(
    void
)
{
    return (pa_mcc_HangUpAll());
}


//--------------------------------------------------------------------------------------------------
/**
 * This function return the Calling Line Identification Restriction (CLIR) status on the specific
 *  call.
 *
 * The output parameter is updated with the CLIR status.
 *    - LE_ON  Disable presentation of own phone number to remote.
 *    - LE_OFF Enable presentation of own phone number to remote.
 *
 * @return
 *    - LE_OK          The function succeed.
 *    - LE_NOT_FOUND   The call reference was not found.
 *    - LE_UNAVAILABLE CLIR status was not set.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_GetCallerIdRestrict
(
    le_mcc_CallRef_t callRef, ///< [IN] The call reference.
    le_onoff_t* clirStatusPtr        ///< [OUT] the Calling Line Identification Restriction (CLIR) status
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return LE_NOT_FOUND;
    }

    if (callPtr->clirStatus == PA_MCC_NO_CLIR)
    {
        LE_INFO("CLIR field was not set");
        return LE_UNAVAILABLE;
    }

    if (clirStatusPtr == NULL)
    {
        LE_KILL_CLIENT("clirStatus is NULL !");
        return LE_FAULT;
    }

    if (callPtr->clirStatus == PA_MCC_ACTIVATE_CLIR)
    {
        *clirStatusPtr = LE_ON;
    }
    else
    {
        *clirStatusPtr = LE_OFF;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function set the Calling Line Identification Restriction (CLIR) status on the specific call.
 * By default the CLIR status is not set.
 *
 * @return
 *     - LE_OK        The function succeed.
 *     - LE_NOT_FOUND The call reference was not found.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_SetCallerIdRestrict
(
    le_mcc_CallRef_t callRef, ///< [IN] The call reference.
    le_onoff_t clirStatus         ///< [IN] The Calling Line Identification Restriction (CLIR) status.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return LE_NOT_FOUND;
    }

    if (clirStatus == LE_ON)
    {
        callPtr->clirStatus = PA_MCC_ACTIVATE_CLIR;
    }
    else
    {
        callPtr->clirStatus = PA_MCC_DEACTIVATE_CLIR;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_mcc_CallEvent'
 *
 * Register an event handler that will be notified when an event occurs on call.
 *
 * @return A reference to the new event handler object.
 *
 * @note It is a fatal error if this function does succeed.  If this function fails, it will not
 *       return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_CallEventHandlerRef_t le_mcc_AddCallEventHandler
(
    le_mcc_CallEventHandlerFunc_t       handlerFuncPtr, ///< [IN] The event handler function.
    void*                                   contextPtr      ///< [IN] The handlers context.
)
{
    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    // search the sessionCtx; create it if doesn't exist
    SessionCtxNode_t* sessionCtxPtr = GetSessionCtx(le_mcc_GetClientSessionRef());

    if (!sessionCtxPtr)
    {
        // Create the session context
        sessionCtxPtr = CreateSessionCtx();
    }

    // Add the handler in the list
    HandlerCtxNode_t * handlerCtxPtr = le_mem_ForceAlloc(HandlerPool);
    handlerCtxPtr->handlerFuncPtr = handlerFuncPtr;
    handlerCtxPtr->userContext = contextPtr;
    handlerCtxPtr->handlerRef = le_ref_CreateRef(HandlerRefMap, handlerCtxPtr);
    handlerCtxPtr->sessionCtxPtr = sessionCtxPtr;
    handlerCtxPtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&sessionCtxPtr->handlerList, &handlerCtxPtr->link);

    return handlerCtxPtr->handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove the registered event handler. Call this function when you no longer wish to be notified of
 * events on calls.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_RemoveCallEventHandler
(
    le_mcc_CallEventHandlerRef_t handlerRef   ///< [IN] The handler object to remove.
)
{
    // Get the hander context
    HandlerCtxNode_t* handlerCtxPtr = le_ref_Lookup(HandlerRefMap, handlerRef);

    if (handlerCtxPtr == NULL)
    {
        LE_ERROR("Invalid reference (%p) provided!", handlerRef);
        return;
    }

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(HandlerRefMap, handlerRef);

    SessionCtxNode_t* sessionCtxPtr = handlerCtxPtr->sessionCtxPtr;

    if (!sessionCtxPtr)
    {
        LE_ERROR("No sessionCtxPtr !!!");
        return;
    }

    // Remove the handler node from the session context
    le_dls_Remove(&(sessionCtxPtr->handlerList), &(handlerCtxPtr->link));
    le_mem_Release(handlerCtxPtr);

    // if no more handler, clean the session context
    if (le_dls_NumLinks(&sessionCtxPtr->handlerList) == 0 )
    {
        le_dls_Link_t* linkPtr = le_dls_Peek(&sessionCtxPtr->callRefList);
        bool deleteSessionCtx = true;

        // Iterate on all call associated with this session
        while( linkPtr )
        {
            CallRefNode_t* CallRefPtr = CONTAINER_OF( linkPtr,
                                                      CallRefNode_t,
                                                      link);
            linkPtr = le_dls_PeekNext(&sessionCtxPtr->callRefList, linkPtr);

            le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, CallRefPtr->callRef);

            if (callPtr)
            {
                // If the callRef was created by another client, we need to remove a reference on this
                // callRef
                if ( !IsCallCreatedByClient(callPtr, sessionCtxPtr->sessionRef) )
                {
                    le_ref_DeleteRef(MccCallRefMap, CallRefPtr->callRef);
                    le_dls_Remove(&sessionCtxPtr->callRefList, &(CallRefPtr->link));
                    le_mem_Release(CallRefPtr);

                    callPtr->refCount--;
                    LE_DEBUG("Release call %p countRef %d", callPtr, callPtr->refCount);

                    LE_FATAL_IF((callPtr->refCount < 0),
                                "Error Release call %p, refCount %d",
                                callPtr, callPtr->refCount);

                    le_mem_Release(callPtr);
                }
                else
                {
                    // a call was created by this client, do not delete its session context
                    LE_DEBUG("Delete the session context");
                    deleteSessionCtx = false;
                }
            }
            else
            {
                LE_ERROR("No valid callPtr !!!");
            }
        }

        if (deleteSessionCtx)
        {
            le_dls_Remove(&SessionCtxList, &(sessionCtxPtr->link));
            le_mem_Release(sessionCtxPtr);
        }
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * This function returns the call identifier number.
 *
 *
 * @return
 *    - LE_OK        The function succeed.
 *    - LE_NOT_FOUND The call reference was not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_GetCallIdentifier
(
    le_mcc_CallRef_t callRef, ///< [IN] The call reference.
    int32_t* callIdPtr             ///< [OUT] Call identifier
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_ERROR("Invalid reference (%p) provided!", callRef);
        return LE_NOT_FOUND;
    }

    *callIdPtr = callPtr->callId;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function activates or deactivates the call waiting service.
 *
 * @return
 *     - LE_OK        The function succeed.
 *     - LE_FAULT     The function failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_SetCallWaitingService
(
    bool active
        ///< [IN] The call waiting activation.
)
{
    return pa_mcc_SetCallWaitingService(active);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function gets the call waiting service status.
 *
 * @return
 *     - LE_OK        The function succeed.
 *     - LE_FAULT     The function failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_GetCallWaitingService
(
    bool* activePtr
        ///< [OUT] The call waiting activation.
)
{
    return pa_mcc_GetCallWaitingService(activePtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * This function activates the specified call. Other calls are placed on hold.
 *
 * @return
 *     - LE_OK        The function succeed.
 *     - LE_FAULT     The function failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_ActivateCall
(
    le_mcc_CallRef_t callRef
        ///< [IN] The call reference.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_ERROR("Invalid reference (%p) provided!", callRef);
        return LE_NOT_FOUND;
    }

    if ((callPtr->event != LE_MCC_EVENT_WAITING) &&
        (callPtr->event != LE_MCC_EVENT_ON_HOLD))
    {
        return LE_FAULT;
    }

    return pa_mcc_ActivateCall(callPtr->callId);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enables/disables the audio AMR Wideband capability.
 *
 * @return
 *     - LE_OK             The function succeeded.
 *     - LE_UNAVAILABLE    The service is not available.
 *     - LE_FAULT          On any other failure.
 *
 * @note The capability setting takes effect immediately and is not persistent to reset.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_SetAmrWbCapability
(
    bool  enable   ///< [IN] True enables the AMR Wideband capability, false disables it.
)
{
    return pa_mcc_SetAmrWbCapability(enable);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the audio AMR Wideband capability.
 *
 * @return
 *     - LE_OK            The function succeeded.
 *     - LE_UNAVAILABLE   The service is not available.
 *     - LE_FAULT         On any other failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_GetAmrWbCapability
(
    bool*  enabledPtr   ///< [OUT] True if AMR Wideband capability is enabled, false otherwise.
)
{
    if (NULL == enabledPtr)
    {
        LE_ERROR("enabledPtr is Null");
        return LE_FAULT;
    }

    return pa_mcc_GetAmrWbCapability(enabledPtr);
}
