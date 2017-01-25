/** @file le_media_local.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_LEMEDIALOCAL_INCLUDE_GUARD
#define LEGATO_LEMEDIALOCAL_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to play a DTMF on a specific audio stream.
 *
 * @return LE_FORMAT_ERROR  The DTMF characters are invalid.
 * @return LE_BUSY          A DTMF playback is already in progress on the playback stream.
 * @return LE_FAULT         The function failed to play the DTMFs.
 * @return LE_OK            The funtion succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_PlayDtmf
(
    le_audio_Stream_t*   streamPtr, ///< [IN] Stream object
    const char*          dtmfPtr,   ///< [IN] The DTMFs to play.
    uint32_t             duration,  ///< [IN] The DTMF duration in milliseconds.
    uint32_t             pause      ///< [IN] The pause duration between tones in milliseconds.
);

//--------------------------------------------------------------------------------------------------
/**
 * Start media service: check the header, start decoder if needed.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Open
(
    le_audio_Stream_t*          streamPtr,          ///< [IN] Stream object
    le_audio_SamplePcmConfig_t* samplePcmConfigPtr  ///< [IN] Sample configuration
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the media service.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_media_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to capture an audio stream.
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Capture
(
    le_audio_Stream_t*          streamPtr,          ///< [IN] Stream object
    le_audio_SamplePcmConfig_t* samplePcmConfigPtr  ///< [IN] Sample configuration
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop an interface.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Stop
(
    le_audio_Stream_t*          streamPtr          ///< [IN] Stream object
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to flush the remaining audio samples.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Flush
(
    le_audio_Stream_t*          streamPtr          ///< [IN] Stream object
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to resume the playback/capture thread.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Resume
(
    le_audio_Stream_t*          streamPtr          ///< [IN] Stream object
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to pause the playback/capture thread.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Pause
(
    le_audio_Stream_t*          streamPtr          ///< [IN] Stream object
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to play audio samples.
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_PlaySamples
(
    le_audio_Stream_t*          streamPtr,          ///< [IN] Stream object
    le_audio_SamplePcmConfig_t* samplePcmConfigPtr  ///< [IN] Sample configuration
);

//--------------------------------------------------------------------------------------------------
/**
 * This function check if a stream is busy.
 *
 * @return false            The stream is unused
 * @return true             The stream is busy
 */
//--------------------------------------------------------------------------------------------------
bool le_media_IsStreamBusy
(
    le_audio_Stream_t*          streamPtr         ///< [IN] Stream object
);


#endif // LEGATO_LEMEDIALOCAL_INCLUDE_GUARD
