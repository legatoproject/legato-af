/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * @page c_mcc_call Modem Call Control
 *
 * @ref mcc_call_interface.h "Click here for the API Reference documentation."
 *
 * <HR>
 *
 * @section le_mcc_call_starting_a_call Starting a Call
 *
 * To initiate a call, create a new call object with a destination telephone
 * number calling the @c le_mcc_profile_CreateCall() function.
 *
 * @c le_mcc_call_Start() must still initiate the call when ready.
 *
 * The le_mcc_call_Start() function initiates a call attempt (it's asynchronous because it can take
 * time for a call to connect).
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
 * - @c le_mcc_call_HangUp() will disconnect this call.
 *
 * When finished with the call object, call le_mcc_call_Delete() to free all the allocated resources
 * associated with the object. This will frees the reference, but remains active if other holders
 * are using it.
 *
 * This code example uses @c CallAndPlay() to dial a phone number, and if successful, play
 * a sound file. Once the file has played, the call hangs up.
 *
 * @code
 *
 * typedef struct
 * {
 *     le_mcc_profile_ObjRef_t profileRef;
 *     le_mcc_call_ObjRef_t    callRef;
 *     char                    filePath[MAX_FILE_PATH_BYTES];
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
 *     le_mcc_call_ObjRef_t callRef,
 *     le_mcc_call_Event_t  event,
 *     void*                contextPtr
 * )
 * {
 *     PlayContext_t*      myContextPtr = (PlayContext_t*)contextPtr;
 *
 *     switch (event)
 *     {
 *         case LE_MCC_CALL_EVENT_CONNECTED:
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
 *         case LE_MCC_CALL_EVENT_TERMINATED:
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
 * @section le_mcc_call_answering_a_call Answering a call
 *
 * Receiving calls is similar sending calls. Add a handler through
 * @c le_mcc_profile_AddCallEventHandler() to be notified of incoming calls.
 *
 * To answer, call @c le_mcc_call_Answer(). To reject it, call @c le_mcc_call_Delete().
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
 *     le_mcc_profile_ObjRef_t profileRef;
 *     le_mcc_call_ObjRef_t    callRef;
 *     uint32_t                calledPartyCount;
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
 *     le_mcc_call_ObjRef_t callRef,
 *     le_mcc_call_Event_t  event,
 *     void*                contextPtr
 * )
 * {
 *     char                tel[TEL_NMBR_MAX_LEN];
 *     VoiceContext_t*     myContextPtr = (VoiceContext_t*)contextPtr;
 *
 *     switch (event)
 *     {
 *         case LE_MCC_CALL_EVENT_INCOMING:
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
 *          case LE_MCC_CALL_EVENT_TERMINATED:
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
 * @section le_mcc_call_ending_all_call Ending all calls
 *
 * A special function can be used to hang-up all the ongoing calls: @c le_mcc_call_HangUpAll().
 * This function can be used to hang-up any calls that have been initiated through another client
 * like AT commands.
 *
 *
 * @todo Supplementary services will be available in a future release. Create simpler examples.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */
/**
 * @file mcc_call_interface.h
 *
 * Legato @ref c_mcc_call include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#ifndef MCC_CALL_INTERFACE_H_INCLUDE_GUARD
#define MCC_CALL_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "userInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_call_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_call_StopClient
(
    void
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
    le_mcc_call_ObjRef_t callRef
        ///< [IN]
        ///< The call object to free.
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
    le_mcc_call_ObjRef_t callRef
        ///< [IN]
        ///< Reference to the call object.
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
    le_mcc_call_ObjRef_t callRef
        ///< [IN]
        ///< The call reference to read.
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
    le_mcc_call_ObjRef_t callRef,
        ///< [IN]
        ///<  The call reference to read from.

    char* telPtr,
        ///< [OUT]
        ///< The telephone number string.

    size_t telPtrNumElements
        ///< [IN]
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
    le_mcc_call_ObjRef_t callRef
        ///< [IN]
        ///< The call reference to read from.
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
    le_mcc_call_ObjRef_t callRef
        ///< [IN]
        ///< The call reference to read from.
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
    le_mcc_call_ObjRef_t callRef
        ///< [IN]
        ///< The call reference to read from.
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
    le_mcc_call_ObjRef_t callRef
        ///< [IN]
        ///< The call reference.
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
    le_mcc_call_ObjRef_t callRef
        ///< [IN]
        ///< The call to end.
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


#endif // MCC_CALL_INTERFACE_H_INCLUDE_GUARD

