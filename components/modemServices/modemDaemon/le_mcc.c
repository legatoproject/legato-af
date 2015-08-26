//--------------------------------------------------------------------------------------------------
/**
 * @file le_mcc.c
 *
 * This file contains the source code of the high level MCC (Modem Call Control) API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
 * Define the maximum number of Call profiles.
 */
//--------------------------------------------------------------------------------------------------
#define MCC_MAX_PROFILE 15

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Call objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MCC_MAX_CALL    20

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
    le_mcc_TerminationReason_t      termination;       ///< Call termination reason
    int32_t                         terminationCode;   ///< Platform specific termination code
    pa_mcc_clir_t                   clirStatus;        ///< Call CLIR status
    le_mcc_CallRef_t                callRef;           ///< The call reference.
    bool                            inProgress;        ///< call in progress
    int16_t                         refCount;          ///< ref count
    le_dls_List_t                   sessionRefList;    ///< Clients sessionRef list
}
le_mcc_Call_t;

//--------------------------------------------------------------------------------------------------
/**
 * SessionRef node structure used for the sessionRef list.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t sessionRef;           ///< client sessionRef
    le_dls_Link_t       link;                 ///< link for SessionRefList
}
SessionRefNode_t;

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
 * Safe Reference Map for Calls objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   MccCallPool;



//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Calls objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t MccCallRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Call handlers counters.
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t   CallHandlerCount;

//--------------------------------------------------------------------------------------------------
/**
 * Call state event handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t CallStateEventId;

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

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(MccCallRefMap, callPtr->callRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Update the reference count of a callRef.
 * The goal is to have enough callRef to provide each handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void UpdateReferenceCount
(
    le_mcc_Call_t* callPtr
)
{
    while (callPtr->refCount < CallHandlerCount)
    {
        callPtr->refCount++;
        le_mem_AddRef(callPtr);
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
    int16_t                         id,
    bool                            getInProgess
)
{
    le_mcc_Call_t* callPtr = NULL;

    le_ref_IterRef_t iterRef = le_ref_GetIterator(MccCallRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        callPtr = (le_mcc_Call_t*) le_ref_GetValue(iterRef);

        // Check callId
        if ( (id != (-1)) && (callPtr->callId == id) &&
             ( (getInProgess && callPtr->inProgress) || !getInProgess ) )
        {
            return callPtr;
        }

        // check phone number
        if ( (strncmp(destinationPtr, callPtr->telNumber, sizeof(callPtr->telNumber)) == 0) &&
             ( (getInProgess && callPtr->inProgress) || !getInProgess ) )
        {
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
    le_mcc_Event_t             event,
    le_mcc_TerminationReason_t termination,
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
    callPtr->termination = termination;
    callPtr->terminationCode = terminationCode;
    callPtr->clirStatus = PA_MCC_DEACTIVATE_CLIR;
    callPtr->inProgress = false;
    callPtr->refCount = 1;
    callPtr->sessionRefList=LE_DLS_LIST_INIT;

    // Create a Safe Reference for this Call object.
    callPtr->callRef = le_ref_CreateRef(MccCallRefMap, callPtr);

    // Update reference count
    UpdateReferenceCount(callPtr);

    return callPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Call Event Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerCallEventHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_mcc_CallRef_t *callRef = reportPtr;
    le_mcc_CallEventHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, *callRef);

    if (callPtr == NULL)
    {
        LE_CRIT("Invalid reference (%p) provided!", *callRef);
        return;
    }

    LE_DEBUG("Send Call Event for [%p] with [%d]",*callRef,callPtr->event);
    clientHandlerFunc(*callRef, callPtr->event, le_event_GetContextPtr());
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

    // Acquire wakeup source on first indication of call
    if (LE_MCC_EVENT_SETUP == dataPtr->event ||
        LE_MCC_EVENT_ORIGINATING == dataPtr->event ||
        LE_MCC_EVENT_INCOMING == dataPtr->event)
    {
        // Note: 3GPP calls have both SETUP and INCOMING states, so
        // this will warn on INCOMING state as a second "stay awake"
        le_pm_StayAwake(WakeupSource);
    }

    // Check if we have already an ongoing callPtr for this call
    callPtr = GetCallObject("", dataPtr->callId, true);

    if (callPtr == NULL)
    {
        // Call not in progress
        // check if a callPtr exists with the same number
        callPtr = GetCallObject(dataPtr->phoneNumber, -1, false);

        if (callPtr == NULL)
        {
            callPtr = CreateCallObject (dataPtr->phoneNumber,
                                        dataPtr->callId,
                                        dataPtr->event,
                                        dataPtr->terminationEvent,
                                        dataPtr->terminationCode);
        }

        callPtr->inProgress = true;
    }
    else
    {
        callPtr->event = dataPtr->event;
        callPtr->termination = dataPtr->terminationEvent;
        callPtr->terminationCode = dataPtr->terminationCode;

        // Update reference count
        UpdateReferenceCount(callPtr);
    }

    // Handle call state transition
    switch (dataPtr->event)
    {
         case LE_MCC_EVENT_TERMINATED:
            // Release wakeup source once call termination is processed
            le_pm_Relax(WakeupSource);
            callPtr->inProgress = false;
        break;
        default:
        break;
    }

    // Call the client's handler
    le_event_Report(CallStateEventId,
                    &callPtr->callRef,
                    sizeof(callPtr->callRef));
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
    le_mcc_Call_t* callPtr = NULL;
    SessionRefNode_t* sessionRefNodePtr;
    le_dls_Link_t* linkPtr;

    le_ref_IterRef_t iterRef = le_ref_GetIterator(MccCallRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        callPtr = (le_mcc_Call_t*) le_ref_GetValue(iterRef);

        // Remove corresponding node from the sessionRefList
        linkPtr = le_dls_Peek(&(callPtr->sessionRefList));

        while (linkPtr != NULL)
        {
            sessionRefNodePtr = CONTAINER_OF(linkPtr, SessionRefNode_t, link);

            linkPtr = le_dls_PeekNext(&(callPtr->sessionRefList), linkPtr);

            // Remove corresponding node from the sessionRefList
            if ( sessionRefNodePtr->sessionRef == sessionRef )
            {
                le_dls_Remove(&(callPtr->sessionRefList),
                    &(sessionRefNodePtr->link));

                le_mem_Release(sessionRefNodePtr);

                callPtr->refCount--;
                le_mem_Release(callPtr);
            }
        }
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

    // Create the Safe Reference Map to use for Call object Safe References.
    MccCallRefMap = le_ref_CreateMap("MccCallMap", MCC_MAX_CALL);

    // Initialize call wakeup source - succeeds or terminates caller
    WakeupSource = le_pm_NewWakeupSource(0, CALL_WAKEUP_SOURCE_NAME);

    CallStateEventId = le_event_CreateId("CallStateEventId", sizeof(le_mcc_CallRef_t));

    // Register a handler function for Call Event indications
    if(pa_mcc_SetCallEventHandler(NewCallEventHandler) != LE_OK)
    {
        LE_CRIT("Add pa_mcc_SetCallEventHandler failed");
        return LE_FAULT;
    }

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler( le_mcc_GetServiceRef(),
                                   CloseSessionEventHandler,
                                   NULL );

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
    if (phoneNumPtr == NULL)
    {
        LE_KILL_CLIENT("phoneNumPtr is NULL !");
        return NULL;
    }

    if(strlen(phoneNumPtr) > (LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1))
    {
        LE_KILL_CLIENT("strlen(phoneNumPtr) > %d", (LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1));
        return NULL;
    }

    // Create the Call object.
    le_mcc_Call_t* mccCallPtr = GetCallObject(phoneNumPtr, -1, false);

    if (mccCallPtr != NULL)
    {
        le_mem_AddRef(mccCallPtr);
        mccCallPtr->refCount++;
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

    // Add the new sessionRef into the the sessionRef list
    le_dls_Queue(&mccCallPtr->sessionRefList,
                    &(newSessionRefPtr->link));

    if (mccCallPtr==NULL)
    {
        LE_ERROR("mccCallPtr null!!!!");
        return NULL;
    }

    LE_DEBUG("Create Call ref.%p", mccCallPtr->callRef);

    // Return a Safe Reference for this Call object.
    return (mccCallPtr->callRef);
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

    if (callPtr->inProgress)
    {
        return LE_FAULT;
    }
    else
    {
        SessionRefNode_t* sessionRefNodePtr;
        le_dls_Link_t* linkPtr;

        callPtr->refCount--;
        LE_DEBUG("refcount %d", callPtr->refCount);

        // Remove corresponding node from the sessionRefList
        linkPtr = le_dls_Peek(&(callPtr->sessionRefList));

        while (linkPtr != NULL)
        {
            sessionRefNodePtr = CONTAINER_OF(linkPtr, SessionRefNode_t, link);
            linkPtr = le_dls_PeekNext(&(callPtr->sessionRefList), linkPtr);

            if ( sessionRefNodePtr->sessionRef == le_mcc_GetClientSessionRef() )
            {
                le_dls_Remove(  &(callPtr->sessionRefList),
                                &(sessionRefNodePtr->link));

                le_mem_Release(sessionRefNodePtr);
            }
        }

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
 * @return LE_OK            Function succeed.
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

    res =pa_mcc_VoiceDial(callPtr->telNumber,
                          callPtr->clirStatus,
                          PA_MCC_ACTIVATE_CUG,
                          &callId);

    callPtr->callId = (int16_t)callId;
    callPtr->inProgress = true;

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
    le_result_t result;
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return LE_NOT_FOUND;
    }

    result = pa_mcc_Answer();
    if (result == LE_OK )
    {
        callPtr->event = LE_MCC_EVENT_CONNECTED;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect, or hang up, the specifed call. Any active call handlers
 * will be notified.
 *
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
        return (pa_mcc_HangUp());
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
 *    - LE_OK        The function succeed.
 *    - LE_NOT_FOUND The call reference was not found.
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
 * Default value is LE_OFF (Enable presentation of own phone number to remote).
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
    le_event_HandlerRef_t  handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("ProfileStateChangeHandler",
                                            CallStateEventId,
                                            FirstLayerCallEventHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    CallHandlerCount++;

    return (le_mcc_CallEventHandlerRef_t)(handlerRef);
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
    CallHandlerCount--;

    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
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
