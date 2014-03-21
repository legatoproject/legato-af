/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * @page c_audio Audio Service
 *
 * @ref le_audio.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * @section le_audio_toc Table of Contents
 *
 *   - @ref le_audio_intro
 *   - @ref le_audio_interfaces
 *   - @ref le_audio_streams
 *   - @ref le_audio_connectors
 *   - @ref le_audio_formats
 *   - @ref le_audio_samples
 *
 * @section le_audio_intro Introduction
 *
 * The Audio API allows to handle audio interfaces, and play or listen supported formats on these
 * interfaces.
 *
 * A given Legato device offers several audio interfaces. The Users must choose the input and output
 * interfaces they want to tie together. The Audio stream related to a particular interface is
 * represented with an 'Audio Stream Reference'.
 *
 * The Users can create their own audio path by connecting several audio streams together thanks to
 * audio connectors. They must create Audio connectors to set up these connections.
 *
 * An audio path can support more than two audio interfaces. For example, a 'basic' output audio
 * path of a voice call imply to connect the Modem Voice Received interface with the Speaker
 * interface but, at the same time, the Modem Voice Received interface can be also connected to a
 * Recorder Device interface.
 *
 * @section le_audio_interfaces Open/Close an Audio Interface
 *
 * The following functions allows the Users to select the desired interface:
 *
 * - le_audio_OpenMic(): it returns an Audio Stream Reference of the analog audio signal coming from
 *                       the microphone input.
 * - le_audio_OpenSpeaker(): it returns an Audio Stream Reference of the analog audio signal routed
 *                           to the Speaker output.
 * - le_audio_OpenUsbRx(): it returns an Audio Stream Reference of the digitized audio signal coming
 *                         from an external device connected via USB Audio Class.
 * - le_audio_OpenUsbTx(): it returns an Audio Stream Reference of the digitized audio signal routed
 *                         to an external device connected via USB Audio Class.
 * - le_audio_OpenPcmRx(): it returns an Audio Stream Reference of the digitized audio signal coming
 *                         from an external device connected via the PCM interface.
 * - le_audio_OpenPcmTx(): it returns an Audio Stream Reference of the digitized audio signal routed
 *                         to an external device connected via the PCM interface.
 * - le_audio_OpenModemVoiceRx(): it returns an Audio Stream Reference of the digitized audio signal
 *                                coming from a voice call. The audio format is negotiated with the
 *                                network when the call is established.
 * - le_audio_OpenModemVoiceTx(): it returns an Audio Stream Reference of the digitized audio signal
 *                                routed to a voice call. The audio format is negotiated with the
 *                                network when the call is established.
 *
 * Note that multiple users can own the same stream at the same time.
 *
 * The le_audio_GetFormat() function can be called to get the audio format of an input or output
 * stream.
 *
 * When the Users have finished using an interface they must call le_audio_Close() to release it. If
 * several Users own the same corresponding stream reference, the interface will be definitively
 * closed only after the last user releases the audio stream.
 *
 * @section le_audio_streams Control an Audio Stream
 *
 * Once the Users get an Audio Stream reference, they can control it with the following functions:
 *
 * - le_audio_SetGain(): it allows the User to adjust the gain of an audio stream (0 means 'muted',
 *                       100 is the maximum gain value).
 * - le_audio_GetGain(): it allows the User to retrieve the gain of an audio stream (0 means
 *                       'muted', 100 is the maximum gain value).
 * - le_audio_Mute(): it allows the User to mute an audio stream.
 * - le_audio_Unmute(): it allows the User to unmute an audio stream.
 *
 * @note The hardware may or may not support the full 0-100 resolution, and if you want to see what
 * was actually set call le_audio_GetGain() after le_audio_SetGain() to get the real value.
 *
 * @section le_audio_connectors Create Audio connectors
 *
 * The Users can create their own audio path by connecting several audio streams together.
 *
 * The le_audio_CreateConnector() function creates a reference to an audio connector.
 *
 * The User can tie an audio stream to a connector by calling the le_audio_Connect()
 * function.
 *
 * The User can remove an audio stream from a connector by calling the le_audio_Disconnect()
 * function.
 *
 * When the Users have finished using a connector, they can delete it with the
 * le_audio_DeleteConnector() function.
 *
 * @section le_audio_formats Specifying audio formats
 *
 * The encoding audio format can be specified to (or retrieved from) the audio interfaces which
 * support this option. The format parameter is a string containing the IANA specified encoding
 * format name for Real-Time protocol (RTP).
 *
 * The complete list of the audio encoding format names can be found on the IANA organization web
 * site (http://www.iana.org).
 *
 * The function that can get the current audio format for an open interface is:
 * - le_audio_GetFormat()
 *
 * @warning Please do not forget to check the list of supported audio formats for your specific
 * platform.
 *
 * Some examples of audio formats:
 * - "L16-8K": Linear PCM 16-bit audio @ 8KHz
 * - "L16-16K": Linear PCM 16-bit audio @ 16KHz
 * - "GSM": European GSM Full Rate @ 8KHz (GSM 06.10)
 * - "GSM-EFR": ITU-T GSM Enhanced Full Rate @ 8KHz (GSM 06.60)
 * - "GSM-HR-08": ITU-T GSM Half Rate @ 8KHz (GSM 06.20)
 * - "AMR": Adaptive Multi-Rate - Full Rate @ 8KHz
 * - "AMR-HR": Adaptive Multi-Rate - Half Rate @ 8KHz
 * - "AMR-WB": Adaptive Multi-Rate Wideband @ 16KHz
 *
 * @note The string is not case sensitive.
 *
 * @section le_audio_samples Code samples
 *
 * In the two following code samples, we illustrate how we can create different audio paths for an
 * incoming voice call.
 *
 * In the first example, the audio path depends on the presence of an USB Audio handset. The
 * @b ConnectVoiceCallAudio() function creates this audio path: if I got an USB Audio handset
 * plugged and fully operationnal, I will redirect the audio of my call to it, otherwise I will
 * redirect it to the in-built microphone and speaker.
 *
 * The @b DisconnectVoiceCallAudio() function simply deletes the audio path.
 *
 * @code
 *
 * le_result_t ConnectVoiceCallAudio
 * (
 *     le_audio_ConnectorRef_t*  audioInputConnectorRefPtr,  // [OUT] The input connector.
 *     le_audio_ConnectorRef_t*  audioOutputConnectorRefPtr, // [OUT] The output connector.
 *     le_audio_StreamRef_t*     mdmRxAudioRefPtr,           // [OUT] The received voice call audio stream.
 *     le_audio_StreamRef_t*     mdmTxAudioRefPtr,           // [OUT] The transmitted voice call audio stream.
 *     le_audio_StreamRef_t*     deviceRxAudioRefPtr,        // [OUT] The received device audio stream.
 *     le_audio_StreamRef_t*     deviceTxAudioRefPtr         // [OUT] The transmitted device audio stream.
 * )
 * {
 *     // I get the audio from the voice call, I don't care of its audio format.
 *     *mdmRxAudioRefPtr = le_audio_OpenModemVoiceRx();
 *     *mdmTxAudioRefPtr = le_audio_OpenModemVoiceTx();
 *
 *     // If I cannot get the audio from the voice call, I return an error.
 *     if ((*mdmRxAudioRefPtr == NULL) || (*mdmTxAudioRefPtr == NULL))
 *     {
 *             // I close the audio interfaces whatever the one has failed to open.
 *             le_audio_Close(*mdmRxAudioRefPtr);
 *             le_audio_Close(*mdmTxAudioRefPtr);
 *             return LE_NOT_POSSIBLE;
 *     }
 *
 *     // I create the audio connectors.
 *     *audioInputConnectorRefPtr = le_audio_CreateConnector();
 *     *audioOutputConnectorRefPtr = le_audio_CreateConnector();
 *
 *     // I verify if my Audio USB handset is plugged and operational before trying to use it.
 *     if(IsMyUSBHandsetPlugged() == TRUE)
 *     {
 *         // I can redirect the audio to my USB handset using linear PCM audio format (PCM 16bits @ 16KHz)
 *         *deviceRxAudioRefPtr = le_audio_OpenUsbRx();
 *         *deviceTxAudioRefPtr = le_audio_OpenUsbTx();
 *
 *         le_audio_Connect(*audioInputConnectorRefPtr, *deviceRxAudioRefPtr);
 *         le_audio_Connect(*audioOutputConnectorRefPtr, *deviceTxAudioRefPtr);
 *     }
 *     else
 *     {
 *         // There is no USB Audio handset, I redirect the audio to the in-built Microphone and Speaker.
 *         *deviceRxAudioRefPtr = le_audio_OpenMic();
 *         *deviceTxAudioRefPtr = le_audio_OpenSpeaker();
 *
 *         le_audio_Connect(*audioInputConnectorRefPtr, *deviceRxAudioRefPtr);
 *         le_audio_Connect(*audioOutputConnectorRefPtr, *deviceTxAudioRefPtr);
 *     }
 *
 *     // I tie the audio from the voice call to the connectors.
 *     le_audio_Connect (*audioInputConnectorRefPtr, *mdmTxAudioRefPtr);
 *     le_audio_Connect (*audioOutputConnectorRefPtr, *mdmRxAudioRefPtr);
 *
 *     return LE_OK;
 * }
 *
 *
 * void DisconnectVoiceCallAudio
 * (
 *     le_audio_ConnectorRef_t  audioInputConnectorRef,  // [IN] The input connector.
 *     le_audio_ConnectorRef_t  audioOutputConnectorRef, // [IN] The output connector.
 *     le_audio_StreamRef_t     mdmRxAudioRef,           // [IN] The received voice call audio stream.
 *     le_audio_StreamRef_t     mdmTxAudioRef,           // [IN] The transmitted voice call audio stream.
 *     le_audio_StreamRef_t     deviceRxAudioRef,        // [IN] The received device audio stream.
 *     le_audio_StreamRef_t     deviceTxAudioRef         // [IN] The transmitted device audio stream.
 * )
 * {
 *     // The call is terminated, I can close its audio interfaces.
 *     le_audio_Close(mdmRxAudioRef);
 *     le_audio_Close(mdmTxAudioRef);
 *
 *     // I close all the device interfaces.
 *     le_audio_Close(deviceRxAudioRef);
 *     le_audio_Close(deviceTxAudioRef);
 *
 *     // I delete the Audio connector references.
 *     le_audio_DeleteConnector(audioInputConnectorRef);
 *     le_audio_DeleteConnector(audioOutputConnectorRef);
 * }
 *
 * @endcode
 *
 * In this second example, we have functions to deal with a new 'Incoming call' event during a call
 * already in progress.
 *
 * When no call is in progress I can use the @b ConnectVoiceCallAudio() function to redirect the
 * call audio to the in-built Microphone and Speaker.
 *
 * If a new call is incoming and if it is condidered as a high priority call, I must mute the audio
 * of the first one and connect the new one to my current audio path. The @b SwitchVoiceCallAudio()
 * function performs these actions.
 *
 * When the high priority call terminates, I can turn back to my previous call and reactivate its
 * audio with the @b SwitchBackVoiceCallAudio() function.
 *
 * @code
 *
 * le_result_t ConnectVoiceCallAudio
 * (
 *     le_audio_ConnectorRef_t*  audioInputConnectorRefPtr,  // [OUT] The input connector.
 *     le_audio_ConnectorRef_t*  audioOutputConnectorRefPtr, // [OUT] The output connector.
 *     le_audio_StreamRef_t*     mdmRxAudioRefPtr,           // [OUT] The received voice call audio stream.
 *     le_audio_StreamRef_t*     mdmTxAudioRefPtr,           // [OUT] The transmitted voice call audio stream.
 *     le_audio_StreamRef_t*     micRefPtr,                  // [OUT] The Microphone stream.
 *     le_audio_StreamRef_t*     speakerRefPtr               // [OUT] The Speaker stream.
 * )
 * {
 *     *mdmRxAudioRefPtr = le_audio_OpenModemVoiceRx();
 *     *mdmTxAudioRefPtr = le_audio_OpenModemVoiceTx();
 *
 *     // If I cannot get the audio from the voice call, I return an error.
 *     if ((*mdmRxAudioRefPtr == NULL) || (*mdmTxAudioRefPtr == NULL))
 *     {
 *             // I close the audio interfaces whatever the one has failed to open.
 *             le_audio_Close(*mdmRxAudioRefPtr);
 *             le_audio_Close(*mdmTxAudioRefPtr);
 *             return LE_NOT_POSSIBLE;
 *     }
 *
 *     *audioInputConnectorRefPtr = le_audio_CreateConnector();
 *     *audioOutputConnectorRefPtr = le_audio_CreateConnector();
 *
 *     // Redirect audio to the in-built Microphone and Speaker.
 *     *speakerRefPtr = le_audio_OpenSpeaker();
 *     *micRefPtr = le_audio_OpenMic();
 *
 *     le_audio_Connect(*audioInputConnectorRefPtr, *micRefPtr);
 *     le_audio_Connect (*audioInputConnectorRefPtr, *mdmTxAudioRefPtr);
 *     le_audio_Connect(*audioOutputConnectorRefPtr, *speakerRefPtr);
 *     le_audio_Connect (*audioOutputConnectorRefPtr, *mdmRxAudioRefPtr);
 *
 *     return LE_OK;
 * }
 *
 *
 * le_result_t SwitchVoiceCallAudio
 * (
 *     le_audio_ConnectorRef_t  audioInputConnectorRef,  // [IN] The input connector.
 *     le_audio_ConnectorRef_t  audioOutputConnectorRef, // [IN] The output connector.
 *     le_audio_StreamRef_t     oldMdmRxAudioRef,        // [IN] The received audio stream of the previous voice call.
 *     le_audio_StreamRef_t     oldMdmTxAudioRef,        // [IN] The transmitted audio stream of the previous voice call.
 *     le_audio_StreamRef_t*    newMdmRxAudioRefPtr,     // [OUT] The received audio stream of the new voice call.
 *     le_audio_StreamRef_t*    newMdmTxAudioRefPtr      // [OUT] The transmitted audio stream of the new voice call.
 * )
 * {
 *     if ((newMdmRxAudioRefPtr == NULL)     ||
 *         (newMdmTxAudioRefPtr == NULL))
 *     {
 *         return LE_BAD_PARAMETER;
 *     }
 *
 *     *newMdmRxAudioRefPtr = le_audio_OpenModemVoiceRx();
 *     *newMdmTxAudioRefPtr = le_audio_OpenModemVoiceTx();
 *
 *     // If I cannot get the audio from the voice call, I return an error.
 *     if ((*newMdmRxAudioRefPtr == NULL) || (*newMdmTxAudioRefPtr == NULL))
 *     {
 *         // I close the audio interfaces whatever the one has failed to open.
 *         le_audio_Close(*newMdmRxAudioRefPtr);
 *         le_audio_Close(*newMdmTxAudioRefPtr);
 *         return LE_NOT_POSSIBLE;
 *     }
 *
 *     // I mute the previous call.
 *     le_audio_Mute(oldMdmRxAudioRef);
 *     le_audio_Mute(oldMdmTxAudioRef);
 *
 *     // I connect the new incoming call.
 *     le_audio_Connect (audioInputConnectorRef, *newMdmTxAudioRefPtr);
 *     le_audio_Connect (audioOutputConnectorRef, *newMdmRxAudioRefPtr);
 *
 *     return LE_OK;
 * }
 *
 *
 * le_result_t SwitchBackVoiceCallAudio
 * (
 *     le_audio_StreamRef_t  oldMdmRxAudioRef, // [IN] The received audio stream of the previous voice call.
 *     le_audio_StreamRef_t  oldMdmTxAudioRef, // [IN] The transmitted audio stream of the previous voice call.
 *     le_audio_StreamRef_t  newMdmRxAudioRef, // [IN] The received audio stream of the new voice call.
 *     le_audio_StreamRef_t  newMdmTxAudioRef  // [IN] The transmitted audio stream  of the new voice call.
 * )
 * {
 *     // I can delete the new call audio interfaces.
 *     le_audio_Close(newMdmRxAudioRef);
 *     le_audio_Close(newMdmTxAudioRef);
 *
 *     // I can re-open the previous call streaming.
 *     if (le_audio_Unmute(oldMdmRxAudioRef) != LE_OK)
 *     {
 *         return LE_FAULT;
 *     }
 *     if (le_audio_Unmute(oldMdmTxAudioRef) != LE_OK)
 *     {
 *         return LE_FAULT;
 *     }
 *
 *     return LE_OK;
 * }
 *
 * @endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
/** @file le_audio.h
 *
 * Legato @ref c_audio include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef AUDIO_INTERFACE_H_INCLUDE_GUARD
#define AUDIO_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "audioUserInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_audio_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_audio_StopClient
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Open the Microphone.
 *
 * @return A Reference to the input audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenMic
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Open the Speakerphone.
 *
 * @return A Reference to the output audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenSpeaker
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Open the received audio stream of an USB audio class.
 *
 * @return A Reference to the input audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenUsbRx
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Open the transmitted audio stream of an USB audio class.
 *
 * @return A Reference to the output audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenUsbTx
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Open the received audio stream of the PCM interface.
 *
 * @return A Reference to the input audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenPcmRx
(
    uint32_t timeslot
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Open the transmitted audio stream of the PCM interface.
 *
 * @return A Reference to the output audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenPcmTx
(
    uint32_t timeslot
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Open the received audio stream of a voice call.
 *
 * @return A Reference to the input audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenModemVoiceRx
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Open the transmitted audio stream of a voice call.
 *
 * @return A Reference to the output audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenModemVoiceTx
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the audio format of an input or output stream.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetFormat
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]

    char* formatPtr,
        ///< [OUT]

    size_t formatPtrNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Close an audio stream.
 * If several Users own the stream reference, the interface will be definitively closed only after
 * the last user closes the audio stream.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_audio_Close
(
    le_audio_StreamRef_t streamRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the Gain value of an input or output stream.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_OUT_OF_RANGE  The gain parameter is not between 0 and 100
 * @return LE_OK            The function succeeded.
 *
 * @note The hardware may or may not support the full 0-100 resolution, and if you want to see what
 * was actually set call le_audio_GetGain() after le_audio_SetGain() to get the real value.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetGain
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]

    uint32_t gain
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Gain value of an input or output stream.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_OK            The function succeeded.
 *
 * @note The hardware may or may not support the full 0-100 resolution, and if you want to see what
 * was actually set call le_audio_GetGain() after le_audio_SetGain() to get the real value.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetGain
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]

    uint32_t* gainPtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------
/**
 * Mute an audio stream.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Mute
(
    le_audio_StreamRef_t streamRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Unmute an audio stream.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Unmute
(
    le_audio_StreamRef_t streamRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Create an audio connector reference.
 *
 * @return A Reference to the audio connector, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_ConnectorRef_t le_audio_CreateConnector
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete an audio connector reference.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_audio_DeleteConnector
(
    le_audio_ConnectorRef_t connectorRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Connect an audio stream to the connector reference.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BUSY          There are insufficient DSP resources available.
 * @return LE_BAD_PARAMETER The connector and/or the audio stream references are invalid.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Connect
(
    le_audio_ConnectorRef_t connectorRef,
        ///< [IN]

    le_audio_StreamRef_t streamRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect an audio stream from the connector reference.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_audio_Disconnect
(
    le_audio_ConnectorRef_t connectorRef,
        ///< [IN]

    le_audio_StreamRef_t streamRef
        ///< [IN]
);


#endif // AUDIO_INTERFACE_H_INCLUDE_GUARD

