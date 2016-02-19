/** @file pa_pcm.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PAPCM_INCLUDE_GUARD
#define LEGATO_PAPCM_INCLUDE_GUARD

#include "pa_audio.h"
#include "le_audio_local.h"
#include "le_media_local.h"

//--------------------------------------------------------------------------------------------------
/**
 * Write PCM frames to sound driver.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_pcm_Write
(
    pcm_Handle_t pcmHandle,                 ///< [IN] Handle of the audio resource given by
                                            ///< pa_pcm_InitPlayback() / pa_pcm_InitCapture()
                                            ///< initialization functions
    char* data,                             ///< [OUT] Write buffer
    uint32_t bufsize                        ///< [IN] Buffer length
);

//--------------------------------------------------------------------------------------------------
/**
 * Read PCM frames from sound driver.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_pcm_Read
(
    pcm_Handle_t pcmHandle,                 ///< [IN] Handle of the audio resource given by
                                            ///< pa_pcm_InitPlayback() / pa_pcm_InitCapture()
                                            ///< initialization functions
    char* data,                             ///< [IN] Read buffer
    uint32_t bufsize                        ///< [IN] Buffer length
);

//--------------------------------------------------------------------------------------------------
/**
 * Close sound driver.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_pcm_Close
(
    pcm_Handle_t pcmHandle                 ///< [IN] Handle of the audio resource given by
                                           ///< pa_pcm_InitPlayback() / pa_pcm_InitCapture()
                                           ///< initialization functions
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the period Size from sound driver.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint32_t pa_pcm_GetPeriodSize
(
    pcm_Handle_t pcmHandle                  ///< [IN] Handle of the audio resource given by
                                            ///< pa_pcm_InitPlayback() / pa_pcm_InitCapture()
                                            ///< initialization functions
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize sound driver for PCM capture.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_pcm_InitCapture
(
    pcm_Handle_t *pcmHandlePtr,             ///< [OUT] Handle of the audio resource, to be used on
                                            ///< further PA PCM functions call
    char* devicePtr,                        ///< [IN] Device to be initialized
    le_audio_SamplePcmConfig_t* pcmConfig   ///< [IN] Samples PCM configuration
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize sound driver for PCM playback.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_pcm_InitPlayback
(
    pcm_Handle_t *pcmHandlePtr,             ///< [OUT] Handle of the audio resource, to be used on
                                            ///< further PA PCM functions call
    char* devicePtr,                        ///< [IN] Device to be initialized
    le_audio_SamplePcmConfig_t* pcmConfig   ///< [IN] Samples PCM configuration
);



#endif // LEGATO_PAPCM_INCLUDE_GUARD
