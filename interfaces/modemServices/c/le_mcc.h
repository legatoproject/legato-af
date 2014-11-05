/**
 * @file le_mcc.h
 *
 * Legato @ref c_mcc include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_MCC_H_INCLUDE_GUARD
#define LEGATO_MCC_H_INCLUDE_GUARD


#include "legato.h"
#include "le_mdm_defs.h"
#include "le_audio.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Enumeration to convey current status of a given profile.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MCC_PROFILE_NOT_AVAILABLE,  ///< This profile is not available.
    LE_MCC_PROFILE_IDLE,           ///< This profile is available, nothing is currently happening on it.
    LE_MCC_PROFILE_FORWARDED,      ///< This profile is currently being forwarded to another number.
    LE_MCC_PROFILE_DND,            ///< This profile is in "Do Not Disturb" mode.
    LE_MCC_PROFILE_IN_USE          ///< This profile is currently in use.
}
le_mcc_profile_State_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Enumeration of the possible events that may be reported to a call event handler.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MCC_CALL_EVENT_INCOMING,    ///< Incoming call attempt (new call).
    LE_MCC_CALL_EVENT_ALERTING,    ///< Far end is now alerting its user (outgoing call).
    LE_MCC_CALL_EVENT_EARLY_MEDIA, ///< Callee has not accepted the call, but a media stream
                              ///< is available.
    LE_MCC_CALL_EVENT_CONNECTED,   ///< Call has been established, and is media is active.
    LE_MCC_CALL_EVENT_TERMINATED,  ///< Call has terminated.
    LE_MCC_CALL_EVENT_ON_HOLD,     ///< Remote party has put the call on hold.
    LE_MCC_CALL_EVENT_TRANSFERED,  ///< Remote party transferred or forwarded the call.
}
le_mcc_call_Event_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Enumeration of the possible reasons for call termination.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MCC_CALL_TERM_NETWORK_FAIL,  ///< Network could not complete the call.
    LE_MCC_CALL_TERM_BAD_ADDRESS,   ///< Remote address could not be resolved.
    LE_MCC_CALL_TERM_BUSY,          ///< Callee is currently busy and cannot take the call.
    LE_MCC_CALL_TERM_LOCAL_ENDED,   ///< Local party ended the call.
    LE_MCC_CALL_TERM_REMOTE_ENDED,  ///< Remote party ended the call.
    LE_MCC_CALL_TERM_NOT_DEFINED,   ///< Undefined reason.
}
le_mcc_call_TerminationReason_t;

//--------------------------------------------------------------------------------------------------
// API type definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type to represent profiles capable sending and receiving calls.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_profile_Obj* le_mcc_profile_ObjRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for Profile State's Changes Handler references.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_profile_StateChangeHandler* le_mcc_profile_StateChangeHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for Call Event Handler references.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_profile_CallEventHandler* le_mcc_profile_CallEventHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for managing active calls.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_call_Obj* le_mcc_call_ObjRef_t;


//--------------------------------------------------------------------------------------------------
// Event handler definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Handler is called whenever the state of a specified profile changes.
 *
 * @param newState    New state profile.
 * @param contextPtr  Whatever context information the event handler may require.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_mcc_profile_StateChangeHandlerFunc_t)
(
    le_mcc_profile_State_t  newState,
    void*                   contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Handler called whenever an event is received by a profile on the device.
 *
 * @param callRef     Call associated with the event.
 * @param event       Call event.
 * @param contextPtr  Context information that the event handler may require.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_mcc_profile_CallEventHandlerFunc_t)
(
    le_mcc_call_ObjRef_t callRef,
    le_mcc_call_Event_t  event,
    void*                contextPtr
);


//--------------------------------------------------------------------------------------------------
// API Function definitions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
// CALL PROFILE.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Access a particular profile by name.
 *
 *  @return The profileRef or NULL if profileName is not found.
 *
 * @note If profil name is too long (max 100 digits), it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_ObjRef_t le_mcc_profile_GetByName
(
    const char* profileNamePtr           ///< [IN] Profile name to search
);

//--------------------------------------------------------------------------------------------------
/**
 * Must be called to release a Call Profile.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_Release
(
    le_mcc_profile_ObjRef_t  profileRef    ///< [IN] Call profile reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Used to determine the current state of a given profile.
 *
 * @return Current state of the profile.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_State_t le_mcc_profile_GetState
(
    le_mcc_profile_ObjRef_t    profileRef   ///< [IN] Profile reference to read
);

//--------------------------------------------------------------------------------------------------
/**
 * Add an event handler for profile state changes.
 *
 * @return Reference to the new event handler object.
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
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove the registered event handler, to no longer receive
 * state change events.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_RemoveStateChangeHandler
(
    le_mcc_profile_StateChangeHandlerRef_t handlerRef   ///< [IN] Handler object to remove
);

//--------------------------------------------------------------------------------------------------
/**
 * Register an event handler to be notified when an event occurs on a call associated with
 * a given profile.
 *
 * Registered handler will receive events for both incoming and outgoing calls.
 *
 * @return A reference to the new event handler object.

 * @note It is a fatal error if this function does succeed.  If this function fails, it will not
 *       return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_CallEventHandlerRef_t le_mcc_profile_AddCallEventHandler
(
    le_mcc_profile_ObjRef_t                profileRef,     ///< [IN] Profile to update
    le_mcc_profile_CallEventHandlerFunc_t  handlerFuncPtr, ///< [IN] Event handler function
    void*                                  contextPtr      ///< [IN] Handler's context
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove the registered event handler to no longer be notified of
 * events on calls.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_RemoveCallEventHandler
(
    le_mcc_profile_CallEventHandlerRef_t handlerRef   ///< [IN] Handler object to remove
);

//--------------------------------------------------------------------------------------------------
// CALL.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Create a new call object with a destination telephone number.
 *
 * @c le_mcc_call_Start() must still initiate the call when ready.
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
    le_mcc_profile_ObjRef_t profileRef,     ///< [IN] Profile for new call.
    const char*             destinationPtr  ///< [IN] Target number to call.
);

//--------------------------------------------------------------------------------------------------
/**
 * Call to free up a call reference.
 *
 * @note This frees up the reference, but remains active if other holders
 *  reference it.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_call_Delete
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] Call object to free
);

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
le_result_t le_mcc_call_Start
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] Reference to the call object.
);

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
    le_mcc_call_ObjRef_t  callRef  ///< [IN] Call reference to read
);

//--------------------------------------------------------------------------------------------------
/**
 * Display the remote party telephone number associated with the call.
 *
 * Output parameter is updated with the telephone number. If the Telephone number string length exceeds
 * the value of 'len' parameter, the LE_OVERFLOW error code is returned and 'telPtr' is used until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'telPtr'.
 * Note  the 'len' parameter sould be at least equal to LE_MDMDEFS_PHONE_NUM_MAX_LEN, otherwise LE_OVERFLOW error code
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
    le_mcc_call_ObjRef_t callRef,    ///< [IN]  Call reference to read
    char*                telPtr,     ///< [OUT] Telephone number string
    size_t               len         ///< [IN]  Telephone number string length
);

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
    le_mcc_call_ObjRef_t callRef   ///< [IN] Call reference to read
);

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the transmitted audio stream. All audio generated on this
 * end of the call is sent on this stream.
 *
 * @return Transmitted audio stream reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_mcc_call_GetTxAudioStream
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] Call reference to read
);

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the received audio stream. All audio received from the
 * other end of the call is received on this stream.
 *
 * @return Received audio stream reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_mcc_call_GetRxAudioStream
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] Call reference to read
);

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
le_result_t le_mcc_call_Answer
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] Call reference
);

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
le_result_t le_mcc_call_HangUp
(
    le_mcc_call_ObjRef_t callRef   ///< [IN] Call to end
);

//--------------------------------------------------------------------------------------------------
/**
 * This function will disconnect, or hang up all the ongoing calls. Any active call handlers will
 * be notified.
 *
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_call_HangUpAll
(
    void
);

#endif   // LEGATO_MCC_H_INCLUDE_GUARD
