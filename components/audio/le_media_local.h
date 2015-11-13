/** @file le_media_local.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_LEMEDIALOCAL_INCLUDE_GUARD
#define LEGATO_LEMEDIALOCAL_INCLUDE_GUARD

#include "le_audio_local.h"

//--------------------------------------------------------------------------------------------------
/**
 * Audio format.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MEDIA_WAVE,
    LE_MEDIA_AMR_NB,
    LE_MEDIA_AMR_WB,
    LE_MEDIA_FORMAT_MAX
}
le_media_Format_t;

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
    pa_audio_SamplePcmConfig_t* samplePcmConfigPtr  ///< [IN] Sample configuration
);

//--------------------------------------------------------------------------------------------------
/**
 * Close media service.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Close
(
    le_audio_Stream_t*          streamPtr          ///< [IN] Stream object
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

#endif // LEGATO_LEMEDIALOCAL_INCLUDE_GUARD
