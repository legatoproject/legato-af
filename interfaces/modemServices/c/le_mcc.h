/**
 * @page c_mcc Modem Call Control API
 * 
 * @ref le_mcc.h "Click here for the API Reference documentation."
 *
 * @ref le_mcc_profiles  <br>
 * @ref le_mcc_starting_a_call <br>
 * @ref le_mcc_answering_a_call <br>
 *
 * 
 * The Modem Call Control (mcc) API uses profiles. Calls can be initiated or
 * received through these profiles. Each profile represents a call type or specific configuration of a
 * given call (e.g., a profile can represent a given cellular modem/SIM combination - if the modem
 * in question supports multiple SIM operation).
 *
 * @section le_mcc_profiles Using Profiles
 *
 * A given Legato device can support multiple profiles, but usually, the MCC API will be
 * configured with a single profile name that will initiate or receive calls.
 *
 * Call @c cle_mcc_profile_GetByName() to access a specific profile by name.
 *
 * @c le_mcc_profile_GetState() API allows the application to get the current state of the profile.
 *
 *  @c le_mcc_profile_AddStateChangeHandler() API installs a handler function that is notified
 *  when the profile's state changes.
 *
 * @c le_mcc_profile_RemoveStateChangeHandler() API uninstalls the handler function.
 *
 * When the Profile object is no longer needed, call @c le_mcc_profile_Release() must be used to
 * release the profile.
 *
 * Here's a profile code sample:
 *
 * @code
 *
 * void ManageMyProfile(void)
 * {
 *
 *     [...]
 *
 *     // Get the Modem profile working on SIM card #1
 *     le_mcc_profile_Ref_t myProfile = le_mcc_profile_GetByName("Modem-Sim1");
 *
 *     // Create and start a call on that profile
 *     le_mcc_call_Ref_t myCall = le_mcc_profile_CreateCall(myProfile, "+18008800800");
 *     le_mcc_call_Start(myCall);
 *
 *     [...]
 *     // Set the profile into 'Do Not Disturb' mode: it will disable it's ablity to accept incoming
 *     // calls.
 *     le_mcc_profile_SetDoNotDisturb(myProfile);
 *
 *     [...]
 *     // Clear the 'Do Not Disturb' mode: the profile gets back to be able to accept incoming calls.
 *     le_mcc_profile_ClearDoNotDisturb(myProfile);
 *
 *     [...]
 *     // Set the profile into 'call forward mode': all calls incoming to this profile will
 *     // automatically be forwarded to another telephone number.
 *     le_mcc_profile_SetForwarding(myProfile, "+18008910910");
 *
 *     [...]
 *
 * }
 *
 * @endcode
 *
 * @section le_mcc_starting_a_call Starting a Call
 *
 * To initiate a call, create a new call object with a destination telephone
 * number calling the @c le_mcc_profile_CreateCall() function.
 * 
 * @c le_mcc_call_Start() must still initiate the call when ready.
 *
 * The le_mcc_call_Start() function initiates a call attempt (it's asynchronous because it can take time for a call to connect).
 *
 * It's essential to register a handler function to get the call events. Use  
 * @c le_mcc_profile_AddCallEventHandler() API to install that handler function.
 * As the call attempt proceeds, the profile's registered call event handler will receive events.
 *
 * The le_mcc_profile_RemoveCallEventHandler() API uninstalls the handler function.
 *
 * The following APIs can be used to manage incoming or outgoing calls:
 * - @c le_mcc_call_GetTerminationReason() - termination reason.
 * - @c le_mcc_call_IsConnected() - connection status.
 * - @c le_mcc_call_GetRemoteTel() - displays remote party telephone number associated with the call.
 * - @c le_mcc_call_GetRxAudioStream() must be called to receive audio stream for this call.
 *  Audio received from the other end of the call uses this stream.
 * - @c le_mcc_call_GetTxAudioStream() must be called to transmit audio stream this call.
 *   Audio generated on this end is sent on this stream.
 * - @c le_mcc_call_HangUp() will disconnect this call. All active calls will be disconnected.
 *
 * When finished with the call object, call le_mcc_call_Delete() to free
 * all the allocated resources associated with the object. This will frees the reference, but remains active
 * if other holders are using it.
 *
 * This code example uses @c CallAndPlay() to dial a phone number, and if successful, play
 * a sound file. Once the file has played, the call hangs up.
 *
 * @code
 *
 * typedef struct
 * {
 *     le_mcc_profile_Ref_t   profileRef;
 *     le_mcc_call_Ref_t      callRef;
 *     char                   filePath[MAX_FILE_PATH_BYTES];
 * }
 * PlayContext_t;
 *
 * void CallAndPlay
 * (
 *     const char* destinationTelPtr,
 *     const char* filePathPtr
 * )
 * {
 *     PlayContext_t* contextPtr = (PlayContext_t*)le_mem_ForceAlloc(PlayContextPoolRef);
 *
 *     le_str_Copy(contextPtr->filePath, filePathPtr, sizeof(contextPtr->filePath), NULL);
 *
 *     contextPtr->profileRef = le_mcc_profile_GetByName("Modem-Sim1");
 *
 *     le_mcc_profile_AddCallEventHandler(contextPtr->profileRef,
 *                                        MyCallEventHandler,
 *                                        contextPtr);
 *
 *     contextPtr->callRef = le_mcc_profile_CreateCall(contextPtr->profileRef, destinationTelPtr);
 *
 *     le_mcc_call_Start(contextPtr->callRef);
 * }
 *
 *
 * static void MyCallEventHandler
 * (
 *     le_mcc_call_Ref_t    callRef,
 *     le_mcc_call_Event_t  event,
 *     void*                contextPtr
 * )
 * {
 *     PlayContext_t*      myContextPtr = (PlayContext_t*)contextPtr;
 *
 *     switch (event)
 *     {
 *         case LE_MCC_EVENT_CONNECTED:
 *             {
 *                 le_audio_StreamRef_t callAudioTxRef = le_mcc_call_GetTxAudioStream(callRef);
 *
 *                 le_audio_PlaySoundFromFile(callAudioTxRef,
 *                                            myContextPtr->filePath,
 *                                            MyAudioFinishedHandler,
 *                                            myContextPtr);
 *             }
 *             break;
 *
 *         case LE_MCC_EVENT_TERMINATED:
 *             le_mcc_call_Delete(callRef);
 *             // I don't release the Profile for further use
 *
 *             le_mcc_profile_RemoveCallEventHandler(myContextPtr->profileRef);
 *
 *             le_mem_Release(myContextPtr);
 *             break;
 *     }
 * }
 *
 *
 * static void MyAudioFinishedHandler
 * (
 *     le_audio_PlayComplete reason,
 *     void*                 contextPtr
 * )
 * {
 *     PlayContext_t* myContextPtr = (PlayContext_t*)contextPtr;
 *
 *     le_mcc_call_HangUp(myContextPtr->callRef);  // This will trigger a TERMINATED event.
 * }
 *
 *
 * @endcode
 *
 *
 * @section le_mcc_answering_a_call Answering a call
 *
 * Receiving calls is similar sending calls. Add a handler through
 * @c le_mcc_profile_AddCallEventHandler() to be notified of incoming calls.
 *
 *To answer, call @c le_mcc_call_Answer(). To reject it, call @c le_mcc_call_Delete().
 *
 * This code example uses @c InstallAutomaticAnswer() to install a call event handler
 * that  automatically answers incoming calls. The handler function verifies the incoming
 * call is permitted (through a predefined list), and then decides whether to answer or terminate it.
 * If a call is already active, it can add the new incoming call creating a conference call.
 *
 * @code
 *
 * typedef struct
 * {
 *     le_mcc_profile_Ref_t   profileRef;
 *     le_mcc_call_Ref_t      callRef;
 *     uint32_t               calledPartyCount;
 * }
 * VoiceContext_t;
 *
 * static le_audio_ConnectorRef_t AudioRxConnectorRef = NULL;
 * static le_audio_ConnectorRef_t AudioTxConnectorRef = NULL;
 *
 *
 * void InstallAutomaticAnswer
 * (
 *     void
 * )
 * {
 *     VoiceContext_t* contextPtr = (VoiceContext_t*)le_mem_ForceAlloc(VoiceContextPoolRef);
 *
 *     contextPtr->profileRef = le_mcc_profile_GetByName("Modem-Sim1");
 *
 *     le_mcc_profile_AddCallEventHandler(contextPtr->profileRef,
 *                                        MyVoiceCallEventHandler,
 *                                        contextPtr);
 * }
 *
 *
 * static void MyVoiceCallEventHandler
 * (
 *     le_mcc_call_Ref_t    callRef,
 *     le_mcc_call_Event_t  event,
 *     void*                contextPtr
 * )
 * {
 *     char                tel[TEL_NMBR_MAX_LEN];
 *     VoiceContext_t*     myContextPtr = (VoiceContext_t*)contextPtr;
 *
 *     switch (event)
 *     {
 *         case LE_MCC_EVENT_INCOMING:
 *             {
 *                 le_mcc_profile_State_t myProfileState = LE_MCC_PROFILE_NOT_AVAILABLE;
 *
 *                 le_mcc_call_GetRemoteTel(callRef, &tel, sizeof(tel));
 *                 if (IsAnAuthorizedIncomingCall(tel) == TRUE)
 *                 {
 *                     le_mcc_profile_GetState(myProfile, &myProfileState);
 *                     if(myProfileState == LE_MCC_PROFILE_IN_USE)
 *                     {
 *                         // Another incoming call, turn this into a x-way conference, mixed locally:
 *                         myContextPtr->calledPartyCount++;
 *
 *                         le_audio_StreamRef_t otherCallRxAudioRef = le_mcc_call_GetRxAudioStream(callRef);
 *                         le_audio_StreamRef_t otherCallTxAudioRef = le_mcc_call_GetTxAudioStream(callRef);
 *
 *                         le_audio_connector_AddStream(AudioRxConnectorRef, otherCallRxAudioRef);
 *                         le_audio_connector_AddStream (AudioTxConnectorRef, otherCallTxAudioRef);
 *                     }
 *                     else if(myProfileState == LE_MCC_PROFILE_IDLE)
 *                     {
 *                         // First incoming call
 *                         myContextPtr->calledPartyCount = 1;
 *
 *                         le_audio_StreamRef_t speakerphoneRef = le_audio_OpenSpeakerphone();
 *                         le_audio_StreamRef_t callRxAudioRef = le_mcc_call_GetRxAudioStream(callRef);
 *                         le_audio_StreamRef_t micphoneRef = le_audio_OpenMicphone();
 *                         le_audio_StreamRef_t callTxAudioRef = le_mcc_call_GetTxAudioStream(callRef);
 *
 *                         AudioRxConnectorRef = le_audio_CreateConnector();
 *                         AudioTxConnectorRef = le_audio_CreateConnector();
 *
 *                         le_audio_connector_AddStream(AudioRxConnectorRef, speakerphoneRef);
 *                         le_audio_connector_AddStream (AudioRxConnectorRef, callRxAudioRef);
 *
 *                         le_audio_connector_AddStream(AudioTxConnectorRef, micphoneRef);
 *                         le_audio_connector_AddStream (AudioTxConnectorRef, callTxAudioRef);
 *                     }
 *
 *                     le_mcc_call_Answer(callRef);
 *                 }
 *                 else
 *                 {
 *                     // Reject the incoming call
 *                     le_mcc_call_Delete(callRef);
 *                 }
 *              }
 *              break;
 *
 *          case LE_MCC_EVENT_TERMINATED:
 *              le_mcc_call_Delete(callRef);
 *
 *              // I delete the Audio connector references if it remains only one called party.
 *              if (myContextPtr->calledPartyCount == 1)
 *              {
 *                  le_audio_DeleteConnector(AudioRxConnectorRef);
 *                  le_audio_DeleteConnector(AudioTxConnectorRef);
 *              }
 *              else
 *              {
 *                  myContextPtr->calledPartyCount--;
 *              }
 *              break;
 *      }
 *  }
 *
 * @endcode
 *
 *
 * @todo Supplementary services will be available in a future release. Create simpler examples.
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------
/**
 * @file le_mcc.h
 *
 * Legato @ref c_mcc include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

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
// API type definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type to represent profiles capable sending and receiving calls.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_Profile* le_mcc_profile_Ref_t;

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
typedef struct le_mcc_Call* le_mcc_call_Ref_t;


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
    le_mcc_call_Ref_t    callRef,
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
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_Ref_t le_mcc_profile_GetByName
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
    le_mcc_profile_Ref_t  profileRef    ///< [IN] Call profile reference
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
    le_mcc_profile_Ref_t    profileRef   ///< [IN] Profile reference to read
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
    le_mcc_profile_Ref_t                     profileRef,      ///< [IN] The profile reference.
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
    le_mcc_profile_Ref_t                profileRef,     ///< [IN] Profile to update
    le_mcc_profile_CallEventHandlerFunc_t  handlerFuncPtr, ///< [IN] Event handler function
    void*                               contextPtr      ///< [IN] Handler's context
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
 */
//--------------------------------------------------------------------------------------------------
le_mcc_call_Ref_t le_mcc_profile_CreateCall
(
    le_mcc_profile_Ref_t profileRef,     ///< [IN] Profile for new call.
    const char*          destinationPtr  ///< [IN] Target number to call.
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
    le_mcc_call_Ref_t callRef   ///< [IN] Call object to free
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
    le_mcc_call_Ref_t callRef   ///< [IN] Reference to the call object.
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
    le_mcc_call_Ref_t  callRef  ///< [IN] Call reference to read
);

//--------------------------------------------------------------------------------------------------
/**
 * Display the remote party telephone number associated with the call.
 *
 * Output parameter is updated with the telephone number. If the Telephone number string length exceeds
 * the value of 'len' parameter, the LE_OVERFLOW error code is returned and 'telPtr' is used until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'telPtr'.
 * Note  the 'len' parameter sould be at least equal to LE_TEL_NMBR_MAX_LEN, otherwise LE_OVERFLOW error code
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
    le_mcc_call_Ref_t callRef,    ///< [IN]  Call reference to read 
    char*             telPtr,     ///< [OUT] Telephone number string
    size_t            len         ///< [IN]  Telephone number string length
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
    le_mcc_call_Ref_t callRef   ///< [IN] Call reference to read 
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
    le_mcc_call_Ref_t callRef   ///< [IN] Call reference to read 
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
    le_mcc_call_Ref_t callRef   ///< [IN] Call reference to read 
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
    le_mcc_call_Ref_t callRef   ///< [IN] Call reference
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
    le_mcc_call_Ref_t callRef   ///< [IN] Call to end
);


#endif   // LEGATO_MCC_H_INCLUDE_GUARD
