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
 * Define the maximum number of Modem profiles.
 */
//--------------------------------------------------------------------------------------------------
#define MCC_MAX_PROFILE 25

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
/**
 *  Enumeration of the call direction.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    OUTGOING_CALL,  ///< Outgoing call.
    INCOMING_CALL,  ///< Incoming call.
}
le_mcc_Direction_t;


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Modem Call Control Profile structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_profile_Obj
{
    char                           name[MCC_PROFILE_NAME_MAX_BYTES]; ///< Name of the profile
    le_mcc_profile_State_t         state;                            ///< State of the profile
    uint32_t                       profileIndex;                     ///< Index of the profile
    le_event_Id_t                  stateChangeEventId;               ///< Profile's state change
                                                                     ///<  Event ID
    le_mcc_call_ObjRef_t           callRef;                          ///< Reference for current
                                                                     ///<  call, if connected
    le_event_Id_t                  callEventId;                      ///< Call Event Id
}
le_mcc_Profile_t;

//--------------------------------------------------------------------------------------------------
/**
 * Modem Call object structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_call_Obj
{
    char                            telNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES]; ///< Telephone number
    le_mcc_Direction_t              direction;                      ///< Call direction
    int16_t                         callId;                         ///< Outgoing call ID
    le_mcc_Profile_t*               profile;                        ///< Profile to which the call
                                                                    ///<  belongs
    le_mcc_call_Event_t             event;                          ///< Last Call event
    le_mcc_call_TerminationReason_t termination;                    ///< Call termination reason
    le_audio_StreamRef_t            txAudioStreamRef;               ///< The transmitted audio
                                                                    ///<  stream reference
    le_audio_StreamRef_t            rxAudioStreamRef;               ///< The received audio stream
                                                                    ///<  reference
    pa_mcc_clir_t                   clirStatus;                     ///< Call CLIR status
}
le_mcc_Call_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data structure to keep a list of the call references.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_CallReference
{
    le_mcc_call_ObjRef_t callRef;     ///< The call reference.
    le_dls_Link_t        link;        ///< Object node link
}
le_mcc_CallReference_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Create and initialize the Call list.
 *
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t CallList;


//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Call Profiles.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   MccProfilePool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Calls.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   MccCallPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Call Profiles objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t MccProfileRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Calls objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t MccCallRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for message references.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   ReferencePool;

//--------------------------------------------------------------------------------------------------
/**
 * Call handlers counters.
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t   CallHandlerCount;


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
 * This table keeps track of the allocated modem profile objects.
 *
 * Since the maximum number of profile objects is known, we can use a table here instead of a
 * linked list.  If a particular entry is NULL, then the profile has not been allocated yet.
 * Since this variable is static, all entries are initially 0 (i.e NULL).
 *
 * The modem profile index is the index into this table +1.
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_Profile_t* ProfileTable[MCC_MAX_PROFILE]; // TODO: Verify if we have a maximum number of profiles


//--------------------------------------------------------------------------------------------------
/**
 * Open Audio
 *
 */
//--------------------------------------------------------------------------------------------------
static void OpenAudio
(
    le_mcc_Call_t* callPtr
)
{
    if(callPtr)
    {
        callPtr->txAudioStreamRef = le_audio_OpenModemVoiceTx();
        callPtr->rxAudioStreamRef = le_audio_OpenModemVoiceRx();

        LE_DEBUG("Open audio.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Close Audio
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseAudio
(
    le_mcc_Call_t* callPtr
)
{
    if(callPtr)
    {
        if (callPtr->rxAudioStreamRef)
        {
            le_audio_Close(callPtr->rxAudioStreamRef);
            callPtr->rxAudioStreamRef = NULL;
        }
        if (callPtr->txAudioStreamRef)
        {
            le_audio_Close(callPtr->txAudioStreamRef);
            callPtr->txAudioStreamRef = NULL;
        }

        LE_DEBUG("Close audio.");
    }
}

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

    if (callPtr)
    {
        if (callPtr->event != LE_MCC_CALL_EVENT_TERMINATED)
        {
            CloseAudio(callPtr);
            pa_mcc_HangUp();
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Profile State Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerProfileStateChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_mcc_profile_State_t* statePtr = reportPtr;
    le_mcc_profile_StateChangeHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    LE_DEBUG("Send Profile State change [%d]",*statePtr);
    clientHandlerFunc(*statePtr, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a call object and return a reference on it.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_CallReference_t* CreateCallObject
(
    le_mcc_Profile_t*               profilePtr,
    const char*                     destinationPtr,
    le_mcc_Direction_t              direction,
    int16_t                         id,
    le_mcc_call_Event_t             event,
    le_mcc_call_TerminationReason_t termination
)
{
    le_mcc_Call_t* callPtr = (le_mcc_Call_t*)le_mem_ForceAlloc(MccCallPool);
    le_utf8_Copy(callPtr->telNumber, destinationPtr, sizeof(callPtr->telNumber), NULL);
    callPtr->profile = profilePtr;
    callPtr->callId = id;
    callPtr->direction = direction;
    callPtr->event = event;
    callPtr->termination = termination;
    callPtr->rxAudioStreamRef = NULL;
    callPtr->txAudioStreamRef = NULL;
    callPtr->clirStatus = PA_MCC_DEACTIVATE_CLIR;

    le_mcc_CallReference_t* newReferencePtr = (le_mcc_CallReference_t*)le_mem_ForceAlloc(ReferencePool);
    // Create a Safe Reference for this Call object.
    newReferencePtr->callRef = le_ref_CreateRef(MccCallRefMap, callPtr);
    newReferencePtr->link = LE_DLS_LINK_INIT;

    // Insert the message in the List.
    le_dls_Queue(&CallList, &(newReferencePtr->link));

    return newReferencePtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the profile object address.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_Profile_t* GetProfile
(
    const char* profileNamePtr
)
{
    // TODO Review how to retrieve the profile
    uint32_t idx;

    for (idx=0; idx<MCC_MAX_PROFILE; idx++)
    {
        if (ProfileTable[idx] != NULL)
        {
            if (!strncmp(ProfileTable[idx]->name, profileNamePtr, sizeof(ProfileTable[idx]->name)))
            {
                le_mem_AddRef((void *)ProfileTable[idx]);
                return (ProfileTable[idx]);
            }
        }
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Verify and update the profile state.
 *
 */
//--------------------------------------------------------------------------------------------------
static void UpdateProfileState
(
    le_mcc_Profile_t*     profilePtr,
    le_mcc_call_Event_t   event
)
{
    bool notify = false;

    if (event == LE_MCC_CALL_EVENT_TERMINATED)
    {
        if (profilePtr->state != LE_MCC_PROFILE_IDLE)
        {
            profilePtr->state = LE_MCC_PROFILE_IDLE;
            notify = true;
        }
    }
    else
    {
        if (profilePtr->state != LE_MCC_PROFILE_IN_USE)
        {
            profilePtr->state = LE_MCC_PROFILE_IN_USE;
            notify = true;
        }
    }

    if(notify)
    {
        // Notify all the registered Profile state handlers
        LE_DEBUG("Notify the Profile State handler (state.%d)", profilePtr->state);
        le_event_Report(profilePtr->stateChangeEventId,
                        (void*)&(profilePtr->state),
                        sizeof(le_mcc_profile_State_t));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Look for an existing Call object and update it.
 *
 * TODO: Manage multiple calls on one profile, for now I manage only one call.
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_call_ObjRef_t UpdateCallObject
(
    pa_mcc_CallEventData_t*  eventPtr,
    le_mcc_Profile_t*        currProfilePtr
)
{
    le_dls_Link_t*          linkPtr;
    le_mcc_CallReference_t* nodePtr = NULL;

    LE_DEBUG("Look for call ID.%d.", eventPtr->callId);

    linkPtr = le_dls_Peek(&CallList);
    while (linkPtr != NULL)
    {
        // Get the node from CallList
        nodePtr = CONTAINER_OF(linkPtr, le_mcc_CallReference_t, link);
        le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, nodePtr->callRef);
        // Should never happen
        LE_FATAL_IF((callPtr == NULL), "Invalid reference (%p) provided!", nodePtr->callRef);
        if (callPtr->callId == (int16_t)eventPtr->callId)
        {
            LE_DEBUG("Found a call (ID.%d).", eventPtr->callId);
            callPtr->profile = currProfilePtr;
            callPtr->event = eventPtr->event;
            callPtr->termination = eventPtr->TerminationEvent;
            if(eventPtr->event == LE_MCC_CALL_EVENT_TERMINATED)
            {
                callPtr->callId = -1;
            }
            return (nodePtr->callRef);
        }

        // Move to the next node.
        linkPtr = le_dls_PeekNext(&CallList, linkPtr);
    }

    return NULL;
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
    le_mcc_call_ObjRef_t *callRef = reportPtr;
    le_mcc_profile_CallEventHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, *callRef);

    if (callPtr == NULL)
    {
        LE_CRIT("Invalid reference (%p) provided!", *callRef);
        return;
    }

    LE_DEBUG("Send Call Event for [%p] with [%d]",*callRef,callPtr->event);
    clientHandlerFunc(*callRef,callPtr->event, le_event_GetContextPtr());
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
    le_mcc_Profile_t* currProfilePtr = NULL;

    if ((currProfilePtr = GetProfile("Modem-Sim1")) == NULL)
    {
        LE_CRIT("Didn't find the profile!");
        return;
    }

    // Acquire wakeup source on first indication of call
    if (LE_MCC_CALL_EVENT_SETUP == dataPtr->event ||
        LE_MCC_CALL_EVENT_ORIGINATING == dataPtr->event ||
        LE_MCC_CALL_EVENT_INCOMING == dataPtr->event)
    {
        // Note: 3GPP calls have both SETUP and INCOMING states, so
        // this will warn on INCOMING state as a second "stay awake"
        le_pm_StayAwake(WakeupSource);
        // Return if SETUP or ORIGINATING, but process INCOMING
        if (LE_MCC_CALL_EVENT_INCOMING != dataPtr->event)
            return;
    }

    // Update Profile State
    UpdateProfileState(currProfilePtr, dataPtr->event);

    // Check if event regards an outgoing or incoming Call
    if ((currProfilePtr->callRef = UpdateCallObject(dataPtr, currProfilePtr)) == NULL)
    {
        // This event is for an incoming call
        if ((dataPtr->event == LE_MCC_CALL_EVENT_INCOMING) && (CallHandlerCount))
        {
            // Create the Call object for new incoming call.
            LE_DEBUG("Create incoming call");
            le_mcc_CallReference_t* newReferencePtr = CreateCallObject (currProfilePtr,
                                                                        dataPtr->phoneNumber,
                                                                        INCOMING_CALL,
                                                                        dataPtr->callId,
                                                                        dataPtr->event,
                                                                        dataPtr->TerminationEvent);

            currProfilePtr->callRef = newReferencePtr->callRef;
        }
    }

    // Handle call state transition
    switch (dataPtr->event)
    {
        case LE_MCC_CALL_EVENT_CONNECTED:
            OpenAudio(le_ref_Lookup(MccCallRefMap, currProfilePtr->callRef));
            break;
        case LE_MCC_CALL_EVENT_TERMINATED:
            CloseAudio(le_ref_Lookup(MccCallRefMap, currProfilePtr->callRef));
            break;
        default :
            break;
    }

    // Call the client's handler
    LE_DEBUG("Notify the Call event handler (callRef.%p)", currProfilePtr->callRef);
    le_event_Report(currProfilePtr->callEventId,
                    &(currProfilePtr->callRef),
                    sizeof(currProfilePtr->callRef));

    // Release wakeup source once call termination is processed
    if (LE_MCC_CALL_EVENT_TERMINATED == dataPtr->event)
        le_pm_Relax(WakeupSource);
}


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the Modem Call Control
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_Init
(
    void
)
{
    le_mcc_Profile_t* profilePtr;
    int               idx;
    char              eventName[32];

    // Create a pool for Profile objects
    MccProfilePool = le_mem_CreatePool("MccProfilePool", sizeof(struct le_mcc_profile_Obj));
    le_mem_ExpandPool(MccProfilePool, MCC_MAX_PROFILE);

    // Create the Safe Reference Map to use for Profile object Safe References.
    MccProfileRefMap = le_ref_CreateMap("MccProfileMap", MCC_MAX_PROFILE);

    // Create a pool for Call objects
    MccCallPool = le_mem_CreatePool("MccCallPool", sizeof(struct le_mcc_call_Obj));
    le_mem_ExpandPool(MccCallPool, MCC_MAX_CALL);
    le_mem_SetDestructor(MccCallPool, CallDestructor);

    // Create the Safe Reference Map to use for Call object Safe References.
    MccCallRefMap = le_ref_CreateMap("MccCallMap", MCC_MAX_CALL);

    // Create a pool for Call references list
    ReferencePool = le_mem_CreatePool("MccReferencePool", sizeof(le_mcc_call_ObjRef_t));

    // BEGIN, TODO Profile creation should not belong to this module
    // Create a default Modem-Sim1 profile
    profilePtr = (struct le_mcc_profile_Obj*)le_mem_ForceAlloc(MccProfilePool);
    LE_FATAL_IF((profilePtr == NULL), "No Space for a new profile !");

    memset(profilePtr, 0, sizeof(*profilePtr));

    // It's okay if the name is truncated, since we use strncmp() in the load function
    le_utf8_Copy(profilePtr->name, "Modem-Sim1", sizeof(profilePtr->name), NULL);

    // Each profile has its own event for reporting state changes
    le_utf8_Copy(eventName, profilePtr->name, sizeof(eventName), NULL);
    le_utf8_Append(eventName, "-StateChangeEvent", sizeof(eventName), NULL);
    profilePtr->stateChangeEventId = le_event_CreateId(eventName, sizeof(le_mcc_profile_State_t));

    // Initialize call wakeup source - succeeds or terminates caller
    WakeupSource = le_pm_NewWakeupSource(0, CALL_WAKEUP_SOURCE_NAME);

    // Init the remaining fields
    profilePtr->callRef = NULL;
    profilePtr->state = LE_MCC_PROFILE_IDLE;

    // Each profile has its own event for reporting call
    le_utf8_Copy(eventName, profilePtr->name, sizeof(eventName), NULL);
    le_utf8_Append(eventName, "-CallEvent", sizeof(eventName), NULL);
    profilePtr->callEventId = le_event_CreateId(eventName, sizeof(le_mcc_call_ObjRef_t));

    // Loop through the table until we find the first free modem profile index.  We are
    // guaranteed to find a free entry, otherwise, we would have already exited above.
    for (idx=0; idx<MCC_MAX_PROFILE; idx++)
    {
        LE_DEBUG("ProfileTable[%i] = %p", idx, ProfileTable[idx])

        if (ProfileTable[idx] == NULL)
        {
            ProfileTable[idx] = profilePtr;
            profilePtr->profileIndex = idx+1;
            break;
        }
    }
    // END TODO

    // Register a handler function for Call Event indications
    LE_FATAL_IF((pa_mcc_SetCallEventHandler(NewCallEventHandler) != LE_OK),
                "Add pa_mcc_SetCallEventHandler failed");
}

//--------------------------------------------------------------------------------------------------
// CALL PROFILE.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Access a particular profile by name.
 *
 *  @return The profileRef or NULL if profileName is not found.
 *
 *  @note If profil name is too long (max 100 digits), it is a fatal error,
 *        the function will not return
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_ObjRef_t le_mcc_profile_GetByName
(
    const char* profileNamePtr           ///< [IN] The name of the profile to search for.
)
{
    int  idx;

    if (profileNamePtr == NULL)
    {
        LE_KILL_CLIENT("profileNamePtr is NULL !");
        return NULL;
    }

    if (strlen(profileNamePtr) > MCC_PROFILE_NAME_MAX_LEN)
    {
        LE_KILL_CLIENT("strlen(profileNamePtr) > %d", MCC_PROFILE_NAME_MAX_LEN);
        return NULL;
    }

    for (idx=0; idx<MCC_MAX_PROFILE; idx++)
    {
        LE_DEBUG("ProfileTable[%i] = %p", idx, ProfileTable[idx])

        if (ProfileTable[idx] != NULL)
        {
            if (!strncmp(ProfileTable[idx]->name, profileNamePtr, sizeof(ProfileTable[idx]->name)))
            {
                le_mem_AddRef((void *)ProfileTable[idx]);
                // Create a Safe Reference for this Profile object.
                return le_ref_CreateRef(MccProfileRefMap, ProfileTable[idx]);
                break;
            }
        }
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release a Call Profile.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_Release
(
    le_mcc_profile_ObjRef_t  profileRef    ///< [IN] The Call profile reference.
)
{
    le_mcc_Profile_t* profilePtr = le_ref_Lookup(MccProfileRefMap, profileRef);

    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", profileRef);
        return;
    }

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(MccProfileRefMap, profileRef);

    le_mem_Release(profilePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to determine the current state of a given profile.
 *
 * @return The current state of the profile.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_State_t le_mcc_profile_GetState
(
    le_mcc_profile_ObjRef_t    profileRef   ///< [IN] The profile reference to read.
)
{
    le_mcc_Profile_t* profilePtr = le_ref_Lookup(MccProfileRefMap, profileRef);

    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", profileRef);
        return LE_MCC_PROFILE_NOT_AVAILABLE;
    }

    return profilePtr->state;
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to add an event handler for profile state changes.
 *
 * @return A reference to the new event handler object.
 *
 * @note It is a fatal error if this function does succeed.  If this function fails, it will not
 *       return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_StateChangeHandlerRef_t le_mcc_profile_AddStateChangeHandler
(
    le_mcc_profile_ObjRef_t                  profileRef,      ///< [IN] The profile reference.
    le_mcc_profile_StateChangeHandlerFunc_t  handlerFuncPtr,  ///< [IN] The event handler function.
    void*                                    contextPtr       ///< [IN] The handlers context.
)
{
    le_event_HandlerRef_t handlerRef;

    le_mcc_Profile_t* profilePtr = le_ref_Lookup(MccProfileRefMap, profileRef);

    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", profileRef);
        return NULL;
    }
    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("ProfileStateChangeHandler",
                                            profilePtr->stateChangeEventId,
                                            FirstLayerProfileStateChangeHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mcc_profile_StateChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove the registered event handler. Call this function when you no longer desire to receive
 * state change events.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_RemoveStateChangeHandler
(
    le_mcc_profile_StateChangeHandlerRef_t handlerRef   ///< [IN] The handler object to remove.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Register an event handler that will be notified when an event occurs on call associated with
 * a given profile.
 *
 * The registered handler will receive events for both incoming and outgoing calls.
 *
 * @return A reference to the new event handler object.
 *
 * @note It is a fatal error if this function does succeed.  If this function fails, it will not
 *       return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_CallEventHandlerRef_t le_mcc_profile_AddCallEventHandler
(
    le_mcc_profile_ObjRef_t                 profileRef,     ///< [IN] The profile to update.
    le_mcc_profile_CallEventHandlerFunc_t   handlerFuncPtr, ///< [IN] The event handler function.
    void*                                   contextPtr      ///< [IN] The handlers context.
)
{
    le_event_HandlerRef_t  handlerRef;
    le_mcc_Profile_t* profilePtr = le_ref_Lookup(MccProfileRefMap, profileRef);

    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", profileRef);
        return NULL;
    }
    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("ProfileStateChangeHandler",
                                            profilePtr->callEventId,
                                            FirstLayerCallEventHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    CallHandlerCount++;
    return (le_mcc_profile_CallEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove the registered event handler. Call this function when you no longer wish to be notified of
 * events on calls.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_RemoveCallEventHandler
(
    le_mcc_profile_CallEventHandlerRef_t handlerRef   ///< [IN] The handler object to remove.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    CallHandlerCount--;
}

//--------------------------------------------------------------------------------------------------
// CALL.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Create a new call object with a destination telephone number.
 *
 * The call is not actually established at this point. It is still up to the caller to call
 * le_mcc_call_Start when ready.
 *
 * @return A reference to the new Call object.
 *
 * @note On failure, the process exits, so you don't have to worry about checking the returned
 *       reference for validity.
 *
 * @note If destination number is too long (max 17 digits), it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_call_ObjRef_t le_mcc_profile_CreateCall
(
    le_mcc_profile_ObjRef_t profileRef,     ///< [IN] The profile to create a new call on.
    const char*             destinationPtr  ///< [IN] The target number we are going to call.
)
{
    le_mcc_Profile_t* profilePtr = le_ref_Lookup(MccProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", profileRef);
        return NULL;
    }

    if (destinationPtr == NULL)
    {
        LE_KILL_CLIENT("destinationPtr is NULL !");
        return NULL;
    }

    if(strlen(destinationPtr) > (LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1))
    {
        LE_KILL_CLIENT("strlen(destinationPtr) > %d", (LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1));
        return NULL;
    }

    // Create the Call object.
    le_mcc_CallReference_t* newReferencePtr = CreateCallObject (profilePtr,
                                                                destinationPtr,
                                                                OUTGOING_CALL,
                                                                -1,
                                                                LE_MCC_CALL_EVENT_TERMINATED,
                                                                LE_MCC_CALL_TERM_NOT_DEFINED);

    LE_DEBUG("Create Call ref.%p", newReferencePtr->callRef);
    // Return a Safe Reference for this Call object.
    return (newReferencePtr->callRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Call to free up a call reference.
 *
 * @note This will free the reference, but not necessarily hang up an active call. If there are
 *       other holders of this reference then the call will remain active.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_call_Delete
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] The call object to free.
)
{
    le_mcc_CallReference_t* nodePtr;
    le_dls_Link_t*          linkPtr;
    le_mcc_Call_t*          callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return;
    }

    linkPtr = le_dls_Peek(&CallList);
    while (linkPtr != NULL)
    {
        // Get the node from Call Ref List
        nodePtr = CONTAINER_OF(linkPtr, le_mcc_CallReference_t, link);
        if (nodePtr->callRef == callRef)
        {
            // Remove the object from the Cal reference List.
            le_dls_Remove(&CallList, &nodePtr->link);
            break;
        }

        // Move to the next node.
        linkPtr = le_dls_PeekNext(&CallList, linkPtr);
    }

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(MccCallRefMap, callRef);

    le_mem_Release(callPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a call attempt.
 *
 * Due to the length of time a call can take to connect, this is an asynchronous call.
 *
 * As the call attempt proceeds, the profile's registered call event handler will receive events.
 *
 * @return LE_OK       The function succeed.
 * @return LE_BUSY     A voice call is already ongoing.
 *
 * @note This is an asynchronous call.  On successful return you only know that a call has been
 *       started. You can not assume that a call has been made at that point.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_call_Start
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] Reference to the call object.
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
bool le_mcc_call_IsConnected
(
    le_mcc_call_ObjRef_t  callRef  ///< [IN] The call reference to read.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return false;
    }

    if (callPtr->event == LE_MCC_CALL_EVENT_CONNECTED)
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
 * The output parameter is updated with the Telephone number. If the Telephone number string exceed
 * the value of 'len' parameter, a LE_OVERFLOW error code is returned and 'telPtr' is filled until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'telPtr'.
 * Note tht 'len' sould be at least equal to LE_MDMDEFS_PHONE_NUM_MAX_BYTES, otherwise LE_OVERFLOW error code
 * will be common.
 *
 * @return LE_OVERFLOW      The Telephone number length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_call_GetRemoteTel
(
    le_mcc_call_ObjRef_t callRef,    ///< [IN]  The call reference to read from.
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
le_mcc_call_TerminationReason_t le_mcc_call_GetTerminationReason
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] The call reference to read from.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return LE_MCC_CALL_TERM_NOT_DEFINED;
    }

    return (callPtr->termination);
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the transmitted audio stream of the call in question. All audio generated on this
 * end of the call is sent on this stream.
 *
 * @return The transmitted audio stream reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_mcc_call_GetTxAudioStream
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] The call reference to read from.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return NULL;
    }

    return le_audio_OpenModemVoiceTx();
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the received audio stream of the call in question. All audio received from the
 * other end of the call is received on this stream.
 *
 * @return The received audio stream reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_mcc_call_GetRxAudioStream
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] The call reference to read from.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return NULL;
    }

    return le_audio_OpenModemVoiceRx();
}

//--------------------------------------------------------------------------------------------------
/**
 *  This function will answer to the incoming call.
 *
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_call_Answer
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] The call reference.
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
        callPtr->event = LE_MCC_CALL_EVENT_CONNECTED;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function will disconnect, or hang up the specified call. Any active call disconnect handlers
 * will be notified.
 *
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_call_HangUp
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] The call to end.
)
{
    le_mcc_Call_t* callPtr = le_ref_Lookup(MccCallRefMap, callRef);

    if (callPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", callRef);
        return LE_NOT_FOUND;
    }

    if (callPtr->event != LE_MCC_CALL_EVENT_TERMINATED)
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
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_call_HangUpAll
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
le_result_t le_mcc_call_GetCallerIdRestrict
(
    le_mcc_call_ObjRef_t callRef, ///< [IN] The call reference.
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
le_result_t le_mcc_call_SetCallerIdRestrict
(
    le_mcc_call_ObjRef_t callRef, ///< [IN] The call reference.
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
