/** @file le_audio.h
 *
 * Legato @ref c_audio include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved.
 */


#ifndef LEGATO_AUDIO_INCLUDE_GUARD
#define LEGATO_AUDIO_INCLUDE_GUARD

#include "legato.h"
#include "le_audio_defs.h"

//--------------------------------------------------------------------------------------------------
/**
 * Reference type for Audio Stream
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_audio_Stream*   le_audio_StreamRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference type for Audio Connector
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_audio_Connector*   le_audio_ConnectorRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Open the Microphone.
 *
 * @return  Reference to the input audio stream, NULL if the function fails.
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
 * @return Reference to the output audio stream, NULL if the function fails.
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
 * @return  Reference to the input audio stream, NULL if the function fails.
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
 * @return Reference to the output audio stream, NULL if the function fails.
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
 * @return Reference to the input audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenPcmRx
(
    uint32_t timeslot  ///< [IN] The time slot number.
);

//--------------------------------------------------------------------------------------------------
/**
 * Open the transmitted audio stream of the PCM interface.
 *
 * @return Reference to the output audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenPcmTx
(
    uint32_t timeslot  ///< [IN] The time slot number.
);

//--------------------------------------------------------------------------------------------------
/**
 * Open the received audio stream of the I2S interface.
 *
 * @return Reference to the input audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenI2sRx
(
    le_audio_I2SChannel_t mode  ///< [IN] The channel mode.
);

//--------------------------------------------------------------------------------------------------
/**
 * Open the transmitted audio stream of the I2S interface.
 *
 * @return Reference to the output audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenI2sTx
(
    le_audio_I2SChannel_t mode  ///< [IN] The channel mode.
);

//--------------------------------------------------------------------------------------------------
/**
 * Open the received audio stream of a voice call.
 *
 * @return Reference to the input audio stream, NULL if the function fails.
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
 * @return Reference to the output audio stream, NULL if the function fails.
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
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER Audio stream reference is invalid.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetFormat
(
    le_audio_StreamRef_t streamRef,  ///< [IN] The audio stream reference.
    char*                formatPtr,  ///< [OUT] The name of the audio encoding as used by the
                                     ///  Real-Time Protocol (RTP), specified by the IANA organisation.
    size_t               len         ///< [IN]  The length of format string.
);

//--------------------------------------------------------------------------------------------------
/**
 * Close an audio stream.
 * If several users own the stream reference, the interface closes only after
 * the last user closes the audio stream.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_audio_Close
(
    le_audio_StreamRef_t streamRef  ///< [IN] Audio stream reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the Gain value of an input or output stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER Audio stream reference is invalid.
 * @return LE_OUT_OF_RANGE  Gain parameter is not between 0 and 100
 * @return LE_OK            Function succeeded.
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
    le_audio_StreamRef_t streamRef,  ///< [IN] Audio stream reference.
    uint32_t             gain        ///< [IN] Gain value [0..100] (0 means 'muted', 100 is the
                                     ///       maximum gain value)
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Gain value of an input or output stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER Audio stream reference is invalid.
 * @return LE_OK            Function succeeded.
 *
 * @note The hardware may or may not support the full 0-100 resolution, and if you want to see what
 * was actually set call le_audio_GetGain() after le_audio_SetGain() to get the real value.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetGain
(
    le_audio_StreamRef_t streamRef,  ///< [IN] Audio stream reference.
    uint32_t            *gainPtr     ///< [OUT] Gain value [0..100] (0 means 'muted', 100 is the
                                     ///        maximum gain value)
);

//--------------------------------------------------------------------------------------------------
/**
 * Mute an audio stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER Audio stream reference is invalid.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Mute
(
    le_audio_StreamRef_t streamRef  ///< [IN] The audio stream reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Unmute an audio stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER Audio stream reference is invalid.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Unmute
(
    le_audio_StreamRef_t streamRef  ///< [IN] Audio stream reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Create an audio connector reference.
 *
 * @return Reference to the audio connector, NULL if the function fails.
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
    le_audio_ConnectorRef_t connectorRef    ///< [IN] Connector reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Connect an audio stream to the connector reference.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BUSY           Insufficient DSP resources available.
 * @return LE_BAD_PARAMETER Connector and/or the audio stream references are invalid.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Connect
(
    le_audio_ConnectorRef_t connectorRef,   ///< [IN] Connector reference.
    le_audio_StreamRef_t    streamRef       ///< [IN] Audio stream reference.
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
    le_audio_ConnectorRef_t connectorRef,   ///< [IN] Connector reference.
    le_audio_StreamRef_t    streamRef       ///< [IN] Audio stream reference.
);


#endif // LEGATO_AUDIO_INCLUDE_GUARD
